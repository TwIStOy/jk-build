// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <string>
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
#include "jk/rules/cc/rules/cc_library_helper.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/utils/array.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

// makefile {{{
std::string MakefileCCLibraryCompiler::Name() const {
  return "Makefile.cc_library";
}

void MakefileCCLibraryCompiler ::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  // Compile a `cc_library` into three `Unix Makefile`s.
  //   1. flags.make: include all flags to compile all source files, some
  //   variables will be defined:
  //     - C_FLAGS
  //     - CXX_FLAGS
  //     - CPP_FLAGS
  //     - CXX_DEFINE
  //     - CXX_INCLUDE
  //     flags.make -> 3 versions(debug, release, profile)
  //   2. toolchain.make: include compiler settings, some variables will be
  //   defined:
  //     - CXX
  //     - AR
  //     - RM
  //   3. build.make: main build file, include how to all sources files
  GenerateFlags(wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);

  GenerateToolchain(
      project,
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get(), rule);

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateBuild(
    core::filesystem::ProjectFileSystem *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    CCLibrary *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto build = std::make_unique<core::output::UnixMakefile>("build.make");

  build->DefineCommon(project);

  build->Include(working_folder.Sub("flags.make").Path,
                 "Include the compile flags for this rule's objectes.", true);
  build->Include(working_folder.Sub("toolchain.make").Path, "", true);

  build->AddTarget("all", {"DEBUG"}, {}, "", true);

  build->AddTarget("jk_force", {}, {}, "This target is always outdated.", true);

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  auto counter = common::Counter();

  // headers
  std::list<std::string> all_dep_headers = MergeDepHeaders(rule, project);
  std::list<uint32_t> progress_num;

  // lint sources first
  for (const auto &filename : source_files) {
    auto source_file = SourceFile::Create(rule, rule->Package, filename);
    if (rule->IsNolint(
            project->Resolve(source_file->FullQualifiedPath()).Stringify())) {
      continue;
    }

    progress_num.push_back(
        LintSourceFile(project, source_file, build.get(), working_folder));
    progress_num.push_back(source_file->ProgressNum);
  }

  auto clean_target = working_folder.Sub("clean").Stringify();
  core::builder::CustomCommandLines clean_statements;

  auto library_progress_num = counter->Next();
  progress_num.push_back(library_progress_num);
  for (const auto &build_type : common::FLAGS_BuildTypes) {
    std::list<std::string> all_objects;

    for (const auto &filename : source_files) {
      auto source_file = SourceFile::Create(rule, rule->Package, filename);

      MakeSourceFile(project, build_type, source_file, all_dep_headers,
                     build.get(), working_folder);
      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder, build_type)
              .Stringify());
    }

    auto library_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    build->AddTarget(library_file.Stringify(), {"jk_force"});
    auto ar_stmt = core::builder::CustomCommandLine::Make({
        "@$(AR)",
        library_file.Stringify(),
    });
    std::copy(std::begin(all_objects), std::end(all_objects),
              std::back_inserter(ar_stmt));

    build->AddTarget(
        library_file.Stringify(), all_objects,
        core::builder::CustomCommandLines::Multiple(
            core::builder::CustomCommandLine::Make(
                {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
                 "--progress-num={}"_format(
                     utils::JoinString(",", progress_num)),
                 "--progress-dir={}"_format(project->BuildRoot),
                 "Linking CXX static library {}"_format(
                     library_file.Stringify())}),
            core::builder::CustomCommandLine::Make(
                {"@$(MKDIR)", library_file.Path.parent_path().string()}),
            ar_stmt));

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"$(RM)", library_file.Stringify()}));
    for (const auto &it : all_objects) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"$(RM)", it}));
    }

    auto build_target = working_folder.Sub(build_type).Sub("build").Stringify();
    build->AddTarget(build_target, {library_file.Stringify()}, {},
                     "Rule to build all files generated by this target.", true);

    build->AddTarget(build_type, {build_target}, {}, "", true);
  }

  build->AddTarget(clean_target, {}, std::move(clean_statements), "", true);

  build->Write(w);

  return build;
}

uint32_t MakefileCCLibraryCompiler::LintSourceFile(
    core::filesystem::ProjectFileSystem *project, SourceFile *source_file,
    core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  auto progress_num = common::Counter()->Next();

  auto print_stmt = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green",
       "--progress-num={}"_format(progress_num),
       "--progress-dir={}"_format(project->BuildRoot),
       "Linting CXX file {}"_format(
           project->Resolve(source_file->FullQualifiedPath()))});
  auto lint_stmt = core::builder::CustomCommandLine::Make(
      {"@$(CPPLINT)",
       project->Resolve(source_file->FullQualifiedPath()).Stringify(),
       ">/dev/null"});
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
    core::filesystem::ProjectFileSystem *project, const std::string &build_type,
    SourceFile *source_file, const std::list<std::string> &headers,
    core::output::UnixMakefile *build,
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
       "--progress-num={}"_format(source_file->ProgressNum),
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
        {"@$(CXX)", "$(CXX_DEFINE)", "$(CXX_INCLUDE)", "$(CXX_FLAGS)",
         "$(CPP_FLAGS)", "$({}_CPP_FLAGS)"_format(build_type), "-o",
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
        {"@$(CXX)", "$(CXX_DEFINE)", "$(CXX_INCLUDE)", "$(CXX_FLAGS)",
         "$(C_FLAGS)", "$({}_C_FLAGS)"_format(build_type), "-o",
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
    JK_THROW(core::JKBuildError(
        "unknown file extension: {}",
        source_file->FullQualifiedPath().Path.extension().string()));
  }
}

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateToolchain(
    core::filesystem::ProjectFileSystem *project, core::writer::Writer *w,
    CCLibrary *rule) const {
  auto makefile =
      std::make_unique<core::output::UnixMakefile>("toolchain.make");

  if (common::FLAGS_platform == common::Platform::k64) {
    makefile->DefineEnvironment("CXX", "g++ -m64");
  } else {
    makefile->DefineEnvironment("CXX", "g++ -m32");
  }

  makefile->DefineEnvironment("LINKER", "g++");

  makefile->DefineEnvironment("AR", "ar rcs");

  makefile->DefineEnvironment("RM", "rm", "The command to remove a file.");

  makefile->DefineEnvironment(
      "CPPLINT", toml::find_or<std::string>(project->Configuration(), "cpplint",
                                            "cpplint"));

  makefile->DefineEnvironment("MKDIR", "mkdir -p");

  auto deps = rule->DependenciesInOrder();
  for (auto dep : deps) {
    for (const auto &[k, v] : dep->ExportedEnvironmentVar()) {
      makefile->DefineEnvironment(
          fmt::format("{}_{}", dep->FullQuotedQualifiedName(), k), v);
    }
  }
  for (const auto &[k, v] : rule->ExportedEnvironmentVar()) {
    makefile->DefineEnvironment(
        fmt::format("{}_{}", rule->FullQuotedQualifiedName(), k), v);
  }

  makefile->Write(w);
  return makefile;
}

// {{{
static std::vector<std::string> COMPILE_FLAGS = {
    "-MMD",
    "-msse3",
    "-fPIC",
    "-fstrict-aliasing",
    "-Wall",
    "-Wextra",
    "-Wtrigraphs",
    "-Wuninitialized",
    "-Wwrite-strings",
    "-Wpointer-arith",
    "-Wredundant-decls",
    "-Wunused",
    "-Wmissing-include-dirs",
    "-Wno-missing-field-initializers",
    "-Werror",
};

static std::vector<std::string> CFLAGS = {
    "-D_GNU_SOURCE", "-Werror-implicit-function-declaration"};

static std::vector<std::string> CPPFLAGS() {
  std::vector<std::string> tpl = {
      "-std=c++11",
      "-Wvla",
      "-Wnon-virtual-dtor",
      "-Woverloaded-virtual",
      "-Wno-invalid-offsetof",
      "-Werror=non-virtual-dtor",
      "-D__STDC_FORMAT_MACROS",
      "-DUSE_SYMBOLIZE",
      "-I.",
      "-isystem",
      ".build/.lib/m{}/include"_format(
          common::FLAGS_platform == common::Platform::k32 ? 32 : 64),
      "-I.build/include",
      "-I.build/pb/c++"};
  return tpl;
}

static std::vector<std::string> DEBUG_CFLAGS_EXTRA = {
    "-O0",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fsanitize=address",
    "-fno-inline",
    "-fno-omit-frame-pointer",
    "-fno-builtin",
    "-fno-optimize-sibling-calls",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> DEBUG_CPPFLAGS_EXTRA = {
    "-O0",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fsanitize=address",
    "-fno-inline",
    "-fno-omit-frame-pointer",
    "-fno-builtin",
    "-fno-optimize-sibling-calls",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
    "-ftest-coverage",
    "-fprofile-arcs"};

static std::vector<std::string> RELEASE_CFLAGS_EXTRA = {
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free3",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> RELEASE_CPPFLAGS_EXTRA = {
    "-DNDEBUG",
    "-DUSE_TCMALLOC=1",
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> PROFILING_CFLAGS_EXTRA = {
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free3",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> PROFILING_CPPFLAGS_EXTRA = {
    "-DNDEBUG",
    "-DUSE_TCMALLOC=1",
    "-DHEAP_PROFILING",
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};
// }}}

#define DEFINE_FLAGS(tag)                                                   \
  makefile->DefineEnvironment(                                              \
      #tag "_C_FLAGS",                                                      \
      utils::JoinString(" ", utils::ConcatArrays(compile_flags, CFLAGS,     \
                                                 tag##_CFLAGS_EXTRA)));     \
                                                                            \
  makefile->DefineEnvironment(                                              \
      #tag "_CPP_FLAGS",                                                    \
      utils::JoinString(" ", utils::ConcatArrays(compile_flags, CPPFLAGS(), \
                                                 tag##_CPPFLAGS_EXTRA)));

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateFlags(
    core::writer::Writer *w, CCLibrary *rule) const {
  auto makefile = std::make_unique<core::output::UnixMakefile>("flags.make");

  auto git_desc = R"(-DGIT_DESC="\"`cd {} && git describe --tags --always`\"")";
  auto compile_flags = COMPILE_FLAGS;
  compile_flags.push_back(fmt::format(git_desc, rule->Package->Path));

  makefile->DefineEnvironment(
      "C_FLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CFlags)));

  makefile->DefineEnvironment(
      "CPP_FLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CppFlags)));

  DEFINE_FLAGS(DEBUG);
  DEFINE_FLAGS(RELEASE);
  DEFINE_FLAGS(PROFILING);

  makefile->DefineEnvironment(
      "CXX_FLAGS",
      utils::JoinString(" ", rule->CxxFlags.begin(), rule->CxxFlags.end()));

  const auto &define = rule->ResolveDefinitions();
  const auto &include = rule->ResolveIncludes();
  makefile->DefineEnvironment(
      "CXX_DEFINE", utils::JoinString(" ", define.begin(), define.end(),
                                      [](const std::string &inc) {
                                        return fmt::format("-D{}", inc);
                                      }));
  makefile->DefineEnvironment(
      "CXX_INCLUDE", utils::JoinString(" ", include.begin(), include.end(),
                                       [](const std::string &inc) {
                                         return fmt::format("-I{}", inc);
                                       }));

  makefile->Write(w);
  return makefile;
}
// }}}

// compile database {{{

std::string CompileDatabaseCCLibraryCompiler::Name() const {
  return "CompileDatabase.cc_library";
}

void CompileDatabaseCCLibraryCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  auto writer =
      wf->Build(project->ProjectRoot.Sub("compile_commands.json").Stringify());

  std::vector<std::string> c_flags;
  std::vector<std::string> cpp_flags;

  // generate flags {{{
  {
    auto define = rule->ResolveDefinitions();
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(cpp_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(c_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
  }
  {
    auto vec = rule->ResolveIncludes();
    std::transform(std::begin(vec), std::end(vec),
                   std::back_inserter(cpp_flags), [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
    std::transform(std::begin(vec), std::end(vec), std::back_inserter(c_flags),
                   [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
  }
  auto _CPPFLAGS = CPPFLAGS();
  std::copy(std::begin(_CPPFLAGS), std::end(_CPPFLAGS),
            std::back_inserter(cpp_flags));
  std::copy(std::begin(CFLAGS), std::end(CFLAGS), std::back_inserter(c_flags));
  std::copy(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
            std::back_inserter(cpp_flags));
  std::copy(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
            std::back_inserter(c_flags));
  std::copy(std::begin(rule->CppFlags), std::end(rule->CppFlags),
            std::back_inserter(cpp_flags));
  std::copy(std::begin(rule->CFlags), std::end(rule->CFlags),
            std::back_inserter(c_flags));
  std::copy(std::begin(DEBUG_CPPFLAGS_EXTRA), std::end(DEBUG_CPPFLAGS_EXTRA),
            std::back_inserter(cpp_flags));
  std::copy(std::begin(DEBUG_CFLAGS_EXTRA), std::end(DEBUG_CFLAGS_EXTRA),
            std::back_inserter(c_flags));
  // }}}

  auto sources = rule->ExpandSourceFiles(project, expander);
  std::vector<json> res;
  std::for_each(
      std::begin(sources), std::end(sources),
      [&working_folder, rule, project, &cpp_flags, &c_flags,
       &writer](const std::string &filename) {
        auto sf = SourceFile::Create(rule, rule->Package, filename);

        json res;
        res["file"] =
            project->Resolve(rule->Package->Path).Sub(filename).Stringify();
        std::vector<std::string> command{"g++"};
        if (sf->IsCppSourceFile()) {
          std::copy(std::begin(cpp_flags), std::end(cpp_flags),
                    std::back_inserter(command));
        } else {
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
