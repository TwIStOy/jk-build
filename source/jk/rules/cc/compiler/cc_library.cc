// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/utils/array.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

core::filesystem::JKProject *IncludesResolvingContextImpl::Project() const {
  return project_;
}

static auto logger = utils::Logger("compiler.cc_library");
std::vector<std::string> GlobalDefines;

static std::string FixCpp20Gcc2ClangFlags(const std::string &flag) {
  if (flag == "-fcoroutines") {
    return "-fcoroutines-ts";
  }
  return flag;
}

static std::string FixCpp20Clang2GccFlags(const std::string &flag) {
  if (flag == "-fcoroutines-ts") {
    return "-fcoroutines";
  }
  return flag;
}

// makefile {{{
std::string MakefileCCLibraryCompiler::Name() const {
  return "Makefile.cc_library";
}

void MakefileCCLibraryCompiler ::Compile(
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  // Compile a `cc_library` into three `Unix Makefile`s.
  //   1. flags.make: include all flags to compile all source files, some
  //   variables will be defined:
  //     - WORKING_FOLDER
  //     - CFLAGS
  //     - CXXFLAGS
  //     - CPPFLAGS
  //     - CPP_DEFINES
  //     - CPP_INCLUDES
  //     - DEBUG_CFLAGS
  //     - DEBUG_CXXFLAGS
  //     - RELEASE_CFLAGS
  //     - RELEASE_CXXFLAGS
  //     - PROFILING_CFLAGS
  //     - PROFILING_CXXFLAGS
  //     flags.make -> 3 versions(debug, release, profile)
  //   2. toolchain.make: include compiler settings, some variables will be
  //   defined:
  //     - CXX
  //     - AR
  //     - RM
  //   3. build.make: main build file, include how to all sources files
  GenerateFlags(project,
                wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);

  GenerateToolchain(
      project,
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get(), rule);

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateBuild(
    core::filesystem::JKProject *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    CCLibrary *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto build = std::make_unique<core::output::UnixMakefile>("build.make");

  build->DefineCommon(project);

  build->Include(working_folder.Sub("flags.make").Path,
                 "Include the compile flags for this rule's objectes.", true);
  build->Include(working_folder.Sub("toolchain.make").Path, "", true);

  build->AddTarget("all", {"DEBUG"}, {}, "", true);
  build->DefaultTarget("all");

  build->AddTarget("jk_force", {}, {}, "This target is always outdated.", true);

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  // lint sources first
  for (const auto &filename : source_files) {
    auto source_file = SourceFile::Create(rule, rule->Package, filename);

    if (rule->IsNolint(
            project->Resolve(source_file->FullQualifiedPath()).Stringify())) {
      continue;
    }

    LintSourceFile(project, rule, source_file, build.get(), working_folder);
  }

  // lint headers
  const auto &header_files = rule->ExpandedHeaderFiles(project, expander);
  logger->debug("header files for rule {}: [{}]", rule->FullQualifiedName(),
                utils::JoinString(", ", header_files));
  std::list<std::string> lint_header_targets;
  for (const auto &filename : header_files) {
    auto source_file = SourceFile::Create(rule, rule->Package, filename);
    if (rule->IsNolint(project->Resolve(source_file->FullQualifiedPath()))) {
      continue;
    }

    LintSourceFile(project, rule, source_file, build.get(), working_folder);
    lint_header_targets.push_back(
        source_file->FullQualifiedLintPath(working_folder));
  }

  core::builder::CustomCommandLines clean_statements;

  auto library_progress_num = rule->KeyNumber(".library");
  for (const auto &build_type : common::FLAGS_BuildTypes) {
    std::list<std::string> all_objects;

    /*
     * all_objects.insert(all_objects.end(), std::begin(lint_header_targets),
     *                    std::end(lint_header_targets));
     */

    for (const auto &filename : source_files) {
      auto source_file = SourceFile::Create(rule, rule->Package, filename);

      MakeSourceFile(project, rule, build_type, source_file, {}, build.get(),
                     working_folder);
      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder, build_type)
              .Stringify());
    }

    auto library_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    // build->AddTarget(library_file.Stringify(), {"jk_force"});
    build->AddTarget(library_file.Stringify(), {});
    auto ar_stmt = core::builder::CustomCommandLine::Make({
        "@$(AR)",
        library_file.Stringify(),
    });
    std::copy(std::begin(all_objects), std::end(all_objects),
              std::back_inserter(ar_stmt));

    auto binary_deps = all_objects;
    binary_deps.insert(binary_deps.end(), std::begin(lint_header_targets),
                       std::end(lint_header_targets));
    binary_deps.push_back(working_folder.Sub("build.make"));

    build->AddTarget(
        library_file.Stringify(), binary_deps,
        core::builder::CustomCommandLines::Multiple(
            core::builder::CustomCommandLine::Make(
                {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
                 "--progress-num={}"_format(
                     utils::JoinString(",", rule->KeyNumbers())),
                 "--progress-dir={}"_format(project->BuildRoot),
                 "Linking CXX static library {}"_format(
                     library_file.Stringify())}),
            core::builder::CustomCommandLine::Make(
                {"@$(MKDIR)", library_file.Path.parent_path().string()}),
            ar_stmt));

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"$(RM)", library_file.Stringify()}));
    utils::CopyArray(
        std::begin(all_objects), std::end(all_objects), &clean_statements,
        [](const auto &str) {
          return core::builder::CustomCommandLine::Make({"$(RM)", str});
        });
    utils::CopyArray(
        std::begin(lint_header_targets), std::end(lint_header_targets),
        &clean_statements, [](const auto &str) {
          return core::builder::CustomCommandLine::Make({"$(RM)", str});
        });

    auto build_target = working_folder.Sub(build_type).Sub("build").Stringify();
    build->AddTarget(build_target, {library_file.Stringify()}, {},
                     "Rule to build all files generated by this target.", true);

    build->AddTarget(build_type, {build_target}, {}, "", true);
  }

  build->AddTarget("clean", {}, std::move(clean_statements), "", true);

  build->Write(w);

  return build;
}

uint32_t MakefileCCLibraryCompiler::LintSourceFile(
    core::filesystem::JKProject *project, CCLibrary *rule,
    SourceFile *source_file, core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  auto progress_num =
      rule->KeyNumber(source_file->FullQualifiedPath().Stringify() + "/lint");

  auto file_type = source_file->IsHeaderFile() ? "Header" : "CXX";

  auto print_stmt = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green",
       "--progress-num={}"_format(progress_num),
       "--progress-dir={}"_format(project->BuildRoot),
       "Linting {} file {}"_format(
           file_type, project->Resolve(source_file->FullQualifiedPath()))});

  using core::builder::operator""_c_raw;
  auto lint_stmt = core::builder::CustomCommandLine::Make(
      {"@$(CPPLINT)",
       project->Resolve(source_file->FullQualifiedPath()).Stringify(),
       ">/dev/null"_c_raw});
  auto mkdir_stmt = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)", source_file->FullQualifiedLintPath(working_folder)
                        .Path.parent_path()
                        .string()});
  auto touch_stmt = core::builder::CustomCommandLine::Make(
      {"@touch",
       source_file->FullQualifiedLintPath(working_folder).Stringify()});

  build->AddTarget(
      source_file->FullQualifiedLintPath(working_folder).Stringify(),
      {
          project->Resolve(source_file->FullQualifiedPath()).Stringify(),
          working_folder.Sub("toolchain.make").Stringify(),
      },
      core::builder::CustomCommandLines::Multiple(print_stmt, lint_stmt,
                                                  mkdir_stmt, touch_stmt));
  return progress_num;
}

void MakefileCCLibraryCompiler::MakeSourceFile(
    core::filesystem::JKProject *project, CCLibrary *rule,
    const std::string &build_type, SourceFile *source_file,
    const std::list<std::string> &headers, core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  std::list<std::string> deps{working_folder.Sub("flags.make").Stringify(),
                              working_folder.Sub("toolchain.make").Stringify()};
  if (!source_file->Rule->Downcast<CCLibrary>()->IsNolint(
          project->Resolve(source_file->FullQualifiedPath()))) {
    deps.push_back(
        source_file->FullQualifiedLintPath(working_folder).Stringify());
  }

  build->AddTarget(
      source_file->FullQualifiedObjectPath(working_folder, build_type)
          .Stringify(),
      deps);
  build->Include(source_file->FullQualifiedDotDPath(working_folder, build_type)
                     .Stringify());

  auto print_stmt = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green",
       "--progress-num={}"_format(
           rule->KeyNumber(source_file->FullQualifiedPath())),
       "--progress-dir={}"_format(project->BuildRoot),
       "Building CXX object {}"_format(
           source_file->FullQualifiedObjectPath(working_folder, build_type))});

  auto dep = headers;
  dep.push_back(project->Resolve(source_file->FullQualifiedPath()).Stringify());
  auto mkdir_stmt = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)",
       source_file->FullQualifiedObjectPath(working_folder, build_type)
           .Path.parent_path()
           .string()});

  if (source_file->IsCppSourceFile()) {
    auto build_stmt = core::builder::CustomCommandLine::Make(
        {"@$(CXX)", "$(CPP_DEFINES)", "$(CPP_INCLUDES)", "$(CPPFLAGS)",
         "$(CXXFLAGS)", "$({}_CXXFLAGS)"_format(build_type), "-o",
         source_file->FullQualifiedObjectPath(working_folder, build_type)
             .Stringify(),
         "-c", project->Resolve(source_file->FullQualifiedPath()).Stringify()});

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder, build_type)
            .Stringify(),
        dep,
        core::builder::CustomCommandLines::Multiple(print_stmt, mkdir_stmt,
                                                    build_stmt));
  } else if (source_file->IsCSourceFile()) {
    auto build_stmt = core::builder::CustomCommandLine::Make(
        {"@$(CC)", "$(CPP_DEFINES)", "$(CPP_INCLUDES)", "$(CPPFLAGS)",
         "$(CFLAGS)", "$({}_CFLAGS)"_format(build_type), "-o",
         source_file->FullQualifiedObjectPath(working_folder, build_type)
             .Stringify(),
         "-c", project->Resolve(source_file->FullQualifiedPath()).Stringify()});

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder, build_type)
            .Stringify(),
        dep,
        core::builder::CustomCommandLines::Multiple(print_stmt, mkdir_stmt,
                                                    build_stmt));
  } else {
    logger->info("unknown file extension: {}",
                 source_file->FullQualifiedPath().Path.extension().string());
  }
}

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateToolchain(
    core::filesystem::JKProject *project, core::writer::Writer *w,
    CCLibrary *rule) const {
  auto makefile =
      std::make_unique<core::output::UnixMakefile>("toolchain.make");

  makefile->DefineEnvironment(
      "CXX",
      fmt::format("{} {}", utils::JoinString(" ", project->Config().cxx),
                  project->Platform == core::filesystem::TargetPlatform::k64
                      ? "-m64"
                      : "-m32"));

  makefile->DefineEnvironment(
      "CC",
      fmt::format("{} {}", utils::JoinString(" ", project->Config().cc),
                  project->Platform == core::filesystem::TargetPlatform::k64
                      ? "-m64"
                      : "-m32"));

  makefile->DefineEnvironment("LINKER", "g++");

  makefile->DefineEnvironment("AR", "ar rcs");

  makefile->DefineEnvironment("RM", "$(JK_COMMAND) delete_file",
                              "The command to remove a file.");

  makefile->DefineEnvironment("CPPLINT", project->Config().cpplint_path);

  makefile->DefineEnvironment("MKDIR", "mkdir -p");

  makefile->Write(w);
  return makefile;
}

static std::vector<std::string> cppincludes(
    core::filesystem::JKProject *project) {
  return {
      "-I.", "-isystem",
      // third party library installed
      ".build/.lib/m{}/include"_format(
          project->Platform == core::filesystem::TargetPlatform::k64 ? "64"
                                                                     : "32"),
      "-I.build/include"};
}

static std::vector<std::string> cxxincludes() {
  // generated pb output
  return {"-I.build/pb/c++"};
}

#define DEFINE_FLAGS(tag)                                                     \
  makefile->DefineEnvironment(                                                \
      #tag "_CFLAGS",                                                         \
      utils::JoinString(                                                      \
          " ", utils::ConcatArrays(compile_flags, project->Config().cflags,   \
                                   cppincludes(project),                      \
                                   project->Config().tag##_cflags_extra)));   \
                                                                              \
  makefile->DefineEnvironment(                                                \
      #tag "_CXXFLAGS",                                                       \
      utils::JoinString(                                                      \
          " ", utils::ConcatArrays(compile_flags, project->Config().cxxflags, \
                                   cppincludes(project), cxxincludes(),       \
                                   project->Config().tag##_cxxflags_extra)));

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateFlags(
    core::filesystem::JKProject *project, core::writer::Writer *w,
    CCLibrary *rule) const {
  auto makefile = std::make_unique<core::output::UnixMakefile>("flags.make");

  auto git_desc = R"(-DGIT_DESC="\"`cd {} && git describe --tags --always`\"")";

  auto compile_flags = project->Config().compile_flags;
  compile_flags.push_back(fmt::format(git_desc, rule->Package->Path));

  // environments
  makefile->DefineEnvironment(
      "WORKING_FOLDER", rule->WorkingFolder(project->BuildRoot).Stringify());

  makefile->DefineEnvironment(
      "CFLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CFlags)));

  makefile->DefineEnvironment(
      "CPPFLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CppFlags)));

  DEFINE_FLAGS(debug);
  DEFINE_FLAGS(release);
  DEFINE_FLAGS(profiling);

  makefile->DefineEnvironment(
      "CXXFLAGS", utils::JoinString(
                      " ", utils::ConcatArrays(rule->CxxFlags, GlobalDefines)));

  IncludesResolvingContextImpl includes_resolving_context(project);

  const auto &define = rule->ResolveDefinitions();
  const auto &include = rule->ResolveIncludes(&includes_resolving_context);
  makefile->DefineEnvironment(
      "CPP_DEFINES", utils::JoinString(" ", define.begin(), define.end(),
                                       [](const std::string &inc) {
                                         return fmt::format("-D{}", inc);
                                       }));
  makefile->DefineEnvironment(
      "CPP_INCLUDES", utils::JoinString(" ", include.begin(), include.end(),
                                        [](const std::string &inc) {
                                          return fmt::format("-I{}", inc);
                                        }));

  {
    std::unordered_set<std::string> record;
    rule->RecursiveExecute(
        [mk = makefile.get(), project](const auto *rule) {
          for (const auto &[k, v] : rule->ExportedEnvironmentVar(project)) {
            mk->DefineEnvironment(
                fmt::format("{}_{}",
                            rule->FullQuotedQualifiedNameWithNoVersion(), k),
                v);
          }
        },
        &record);
  }

  makefile->Write(w);
  return makefile;
}
// }}}

// compile database {{{

std::string CompileDatabaseCCLibraryCompiler::Name() const {
  return "CompileDatabase.cc_library";
}

void CompileDatabaseCCLibraryCompiler::Compile(
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  auto writer =
      wf->Build(project->ProjectRoot.Sub("compile_commands.json").Stringify());

  std::vector<std::string> c_flags;
  std::vector<std::string> cxx_flags;

  // generate flags {{{
  {
    auto define = rule->ResolveDefinitions();
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(cxx_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(c_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
  }
  {
    IncludesResolvingContextImpl includes_resolving_context(project);
    auto vec = rule->ResolveIncludes(&includes_resolving_context);
    std::transform(std::begin(vec), std::end(vec),
                   std::back_inserter(cxx_flags), [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
    std::transform(std::begin(vec), std::end(vec), std::back_inserter(c_flags),
                   [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
  }

  cxx_flags = utils::ConcatArrays(cxx_flags, project->Config().cxxflags,
                                  cppincludes(project), cxxincludes());
  c_flags = utils::ConcatArrays(c_flags, project->Config().cflags,
                                cppincludes(project));

  utils::CopyArray(std::begin(rule->CppFlags), std::end(rule->CppFlags),
                   &cxx_flags, &FixCpp20Gcc2ClangFlags);

  utils::CopyArray(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
                   &cxx_flags, &FixCpp20Gcc2ClangFlags);

  std::copy(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
            std::back_inserter(cxx_flags));

  std::copy(std::begin(rule->CFlags), std::end(rule->CFlags),
            std::back_inserter(c_flags));

  std::copy(std::begin(project->Config().debug_cxxflags_extra),
            std::end(project->Config().debug_cxxflags_extra),
            std::back_inserter(cxx_flags));

  std::copy(std::begin(project->Config().debug_cflags_extra),
            std::end(project->Config().debug_cflags_extra),
            std::back_inserter(c_flags));
  // }}}

  auto sources = rule->ExpandSourceFiles(project, expander);
  std::vector<json> res;
  std::for_each(
      std::begin(sources), std::end(sources),
      [&working_folder, rule, project, &cxx_flags, &c_flags,
       &writer](const std::string &filename) {
        auto sf = SourceFile::Create(rule, rule->Package, filename);

        json res;
        res["file"] =
            project->Resolve(rule->Package->Path).Sub(filename).Stringify();
        std::vector<std::string> command;
        if (sf->IsCppSourceFile()) {
          command.push_back("g++");
          std::copy(std::begin(cxx_flags), std::end(cxx_flags),
                    std::back_inserter(command));
          // FIX g++ option '-fcoroutines' in clang toolchains
          command.push_back("-D__cpp_impl_coroutine");
        } else {
          command.push_back("gcc");
          std::copy(std::begin(c_flags), std::end(c_flags),
                    std::back_inserter(command));
        }
        command.push_back("-o");
        command.push_back(
            sf->FullQualifiedObjectPath(working_folder, "DEBUG").Stringify());
        command.push_back("-c");
        command.push_back(
            project->Resolve(rule->Package->Path).Sub(filename).Stringify());
        res["arguments"] = command;
        res["directory"] = project->ProjectRoot.Stringify();

        writer->WriterJSON(res);
      });

  writer->Flush();
}

// }}}

}  // namespace jk::rules::cc

// vim: fdm=marker
