// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/cc_library_compiler.hh"

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/generators/makefile.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "jk/impls/models/cc/source_file.hh"
#include "jk/impls/rules/cc_library.hh"
#include "range/v3/algorithm/contains.hpp"
#include "range/v3/all.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

static auto logger = utils::Logger("compiler.makefile.cc_library");

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

auto CCLibraryCompiler::Name() const -> std::string_view {
  return "makefile.cc_library";
}

auto CCLibraryCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    core::models::BuildRule *rule) const -> void {
  (void)scc;
  auto cc = dynamic_cast<rules::CCLibrary *>(rule);
  DoCompile(session, scc, cc);
}

auto CCLibraryCompiler::DoCompile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    rules::CCLibrary *rule) const -> void {
  auto working_folder = rule->WorkingFolder;

  logger->info("compile rule: {}:{}", rule->Package->Name, rule->Base->Name);

  generate_flag_file(session, working_folder, rule);

  generate_toolchain_file(session, working_folder, rule);

  generate_build_file(session, working_folder, scc, rule);
}

#define WORKING_FOLDER  "WORKING_FOLDER"
#define CFLAGS          "CFLAGS"
#define CPPFLAGS        "CPPFLAGS"
#define CXXFLAGS        "CXXFLAGS"
#define CFLAGS_SUFFIX   "_CFLAGS"
#define CXXFLAGS_SUFFIX "_CXXFLAGS"
#define CPP_INCLUDES    "CPP_INCLUDES"
#define CPP_DEFINES     "CPP_DEFINES"

void CCLibraryCompiler::generate_flag_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    rules::CCLibrary *rule) const {
  static constexpr auto git_desc =
      R"(-DGIT_DESC="\"`cd {} && git describe --tags --always`\"")";

  core::generators::Makefile makefile(working_folder.Sub("flags.make"),
                                      {session->WriterFactory.get()});

  auto compile_flags = session->Project->Config().compile_flags;
  compile_flags.push_back(
      fmt::format(git_desc, rule->Package->Path.Stringify()));

  // environments
  makefile.Env(WORKING_FOLDER, working_folder.Stringify());

  makefile.Env(CFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CPPFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CXXFLAGS,
               absl::StrJoin(ranges::views::concat(
                                 ranges::views::all(rule->CxxFlags),
                                 ranges::views::all(session->ExtraFlags)),
                             " "));

  auto cppincludes = ranges::views::concat(
      ranges::views::single("-isystem"),
      ranges::views::single(fmt::format(
          ".build/.lib/m{}/include",
          session->Project->Platform == core::filesystem::TargetPlatform::k64
              ? "64"
              : "32")),
      ranges::views::single("-I.build/include"));

  static auto cxxincludes = ranges::views::single("-I.build/pb/c++");

  makefile.Env(
      "DEBUG" CFLAGS_SUFFIX,
      absl::StrJoin(
          ranges::views::concat(
              compile_flags, session->Project->Config().cflags,
              session->Project->Config().debug_cflags_extra, cppincludes),
          " "));
  makefile.Env(
      "DEBUG" CXXFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Project->Config().cxxflags,
                        session->Project->Config().debug_cxxflags_extra,
                        cppincludes, cxxincludes),
                    " "));

  makefile.Env(
      "RELEASE" CFLAGS_SUFFIX,
      absl::StrJoin(
          ranges::views::concat(
              compile_flags, session->Project->Config().cflags,
              session->Project->Config().release_cflags_extra, cppincludes),
          " "));
  makefile.Env(
      "RELEASE" CXXFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Project->Config().cxxflags,
                        session->Project->Config().release_cxxflags_extra,
                        cppincludes, cxxincludes),
                    " "));

  makefile.Env(
      "PROFILING" CFLAGS_SUFFIX,
      absl::StrJoin(
          ranges::views::concat(
              compile_flags, session->Project->Config().cflags,
              session->Project->Config().profiling_cflags_extra, cppincludes),
          " "));
  makefile.Env(
      "PROFILING" CXXFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Project->Config().cxxflags,
                        session->Project->Config().profiling_cxxflags_extra,
                        cppincludes, cxxincludes),
                    " "));

  makefile.Env(CPP_DEFINES, absl::StrJoin(rule->ResolvedDefines, " "));

  makefile.Env(CPP_INCLUDES, absl::StrJoin(rule->ResolvedIncludes, " "));

  absl::flat_hash_set<uint32_t> visited;
  auto dfs = [&visited, &makefile](core::models::BuildRule *rule, auto &&dfs) {
    for (const auto &[k, v] : rule->ExportedEnvironmentVars) {
      makefile.Env(
          fmt::format("{}_{}",
                      rule->Base->FullQuotedQualifiedNameWithoutVersion, k),
          v);
    }
  };
  dfs(rule, dfs);
}

void CCLibraryCompiler::generate_toolchain_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    rules::CCLibrary *rule) const {
  core::generators::Makefile makefile(working_folder.Sub("toolchain.make"),
                                      {session->WriterFactory.get()});

  makefile.Env(
      "CXX",
      fmt::format(
          "{} {}", absl::StrJoin(session->Project->Config().cxx, " "),
          session->Project->Platform == core::filesystem::TargetPlatform::k64
              ? "-m64"
              : "-m32"));

  makefile.Env(
      "CC",
      fmt::format(
          "{} {}", absl::StrJoin(session->Project->Config().cc, " "),
          session->Project->Platform == core::filesystem::TargetPlatform::k64
              ? "-m64"
              : "-m32"));

  makefile.Env("LINKER", "g++");

  makefile.Env("AR", "ar rcs");

  makefile.Env("RM", "$(JK_COMMAND) delete_file",
               "The command to remove a file.");

  makefile.Env("CPPLINT", session->Project->Config().cpplint_path);

  makefile.Env("MKDIR", "mkdir -p");
}

uint32_t add_source_files_lint_commands(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    rules::CCLibrary *rule, core::generators::Makefile *makefile,
    models::cc::SourceFile *source_file) {
  auto file_type = source_file->IsHeaderFile ? "Header" : "CXX";
  auto full_qualified_path =
      session->Project->Resolve(source_file->FullQualifiedPath).Stringify();

  auto progress_num = rule->Steps.Step(full_qualified_path);

  auto source_file_path = source_file->ResolveFullQualifiedPath(working_folder);
  auto lint_file_path =
      source_file->ResolveFullQualifiedLintPath(working_folder);

  auto print_stmt = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green",
       fmt::format("--progress-num={}", progress_num),
       fmt::format("--progress-dir={}",
                   session->Project->BuildRoot.Stringify()),
       fmt::format("Linting {} file {}", file_type, full_qualified_path)});

  using core::builder::operator""_c_raw;
  auto lint_stmt = core::builder::CustomCommandLine::Make(
      {"@$(CPPLINT)", full_qualified_path, ">/dev/null"_c_raw});
  auto mkdir_stmt = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)", source_file_path.Path.parent_path().string()});
  auto touch_stmt = core::builder::CustomCommandLine::Make(
      {"@touch", lint_file_path.Stringify()});

  auto toolchain_file = working_folder.Sub("toolchain.make").Stringify();
  makefile->Target(
      lint_file_path.Stringify(),
      ranges::views::concat(ranges::views::single(full_qualified_path),
                            ranges::views::single(toolchain_file)),
      ranges::views::concat(ranges::views::single(print_stmt),
                            ranges::views::single(lint_stmt),
                            ranges::views::single(mkdir_stmt),
                            ranges::views::single(touch_stmt)));
  return progress_num;
}

template<ranges::range R>
void add_source_file_commands(core::models::Session *session,
                              const common::AbsolutePath &working_folder,
                              rules::CCLibrary *rule,
                              core::generators::Makefile *makefile,
                              std::string_view build_type,
                              models::cc::SourceFile *source_file, R headers) {
  std::list<std::string> deps{working_folder.Sub("flags.make").Stringify(),
                              working_folder.Sub("toolchain.make").Stringify()};

  auto full_qualified_path = source_file->FullQualifiedPath.Stringify();
  auto source_filename =
      session->Project->Resolve(full_qualified_path).Stringify();

  // if not in nolint.txt, lint file
  if (!rule->InNolint(source_filename)) {
    deps.push_back(
        source_file->ResolveFullQualifiedLintPath(working_folder).Stringify());
  }

  auto object_file =
      source_file->ResolveFullQualifiedObjectPath(working_folder, build_type);

  makefile->Include(
      source_file->ResolveFullQualifiedDotDPath(working_folder).Stringify());
  makefile->Target(object_file.Stringify(), deps,
                   ranges::views::empty<core::builder::CustomCommandLine>);

  auto print_stmt = core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green",
       fmt::format("--progress-num={}", rule->Steps.Step(full_qualified_path)),
       fmt::format("--progress-dir={}",
                   session->Project->BuildRoot.Stringify()),
       fmt::format("Building CXX object {}", object_file.Stringify())});

  auto dep =
      ranges::views::concat(headers, ranges::views::single(source_filename));

  auto mkdir_stmt = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)", object_file.Path.parent_path().string()});

  if (source_file->IsCppSourceFile) {
    auto build_stmt = core::builder::CustomCommandLine::Make(
        {"@$(CXX)", "$(CPP_DEFINES)", "$(CPP_INCLUDES)", "$(CPPFLAGS)",
         "$(CXXFLAGS)", fmt::format("$({}_CXXFLAGS)", build_type), "-o",
         object_file.Stringify(), "-c", source_filename});

    makefile->Target(object_file.Stringify(), dep,
                     core::builder::CustomCommandLines::Multiple(
                         print_stmt, mkdir_stmt, build_stmt));
  } else if (source_file->IsCSourceFile) {
    auto build_stmt = core::builder::CustomCommandLine::Make(
        {"@$(CC)", "$(CPP_DEFINES)", "$(CPP_INCLUDES)", "$(CPPFLAGS)",
         "$(CFLAGS)", fmt::format("$({}_CFLAGS)", build_type), "-o",
         object_file.Stringify(), "-c", source_filename});

    makefile->Target(object_file.Stringify(), dep,
                     core::builder::CustomCommandLines::Multiple(
                         print_stmt, mkdir_stmt, build_stmt));
  } else {
    logger->info("unknown file extension: {}", full_qualified_path);
  }
}

std::vector<std::string> CCLibraryCompiler::lint_headers(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    rules::CCLibrary *rule, core::generators::Makefile *makefile) const {
  std::vector<std::string> lint_header_targets;
  for (const auto &filename : rule->ExpandedHeaderFiles) {
    auto source_file = models::cc::SourceFile(filename, rule);

    if (rule->InNolint(session->Project->Resolve(source_file.FullQualifiedPath)
                           .Stringify())) {
      continue;
    }

    add_source_files_lint_commands(session, working_folder, rule, makefile,
                                   &source_file);
    lint_header_targets.push_back(
        source_file.ResolveFullQualifiedLintPath(working_folder).Stringify());
  }
  return lint_header_targets;
}

std::vector<std::string> CCLibraryCompiler::add_source_files_commands(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    rules::CCLibrary *rule, core::generators::Makefile *makefile,
    std::vector<std::string> *lint_header_targets,
    std::vector<std::unique_ptr<models::cc::SourceFile>> &source_files,
    std::string_view build_type) const {
  std::vector<std::string> all_objects;
  all_objects.reserve(rule->ExpandedSourceFiles.size());

  for (auto &source_file : source_files) {
    if (!source_file->lint) {
      source_file->lint = true;
      if (!rule->InNolint(
              session->Project->Resolve(source_file->FullQualifiedPath)
                  .Stringify())) {
        add_source_files_lint_commands(session, working_folder, rule, makefile,
                                       source_file.get());
      }
    }

    add_source_file_commands(session, working_folder, rule, makefile,
                             build_type, source_file.get(),
                             ranges::views::all(*lint_header_targets));

    if (rule->ExpandedAlwaysCompileFiles.contains(
            session->Project->Resolve(source_file->FullQualifiedPath)
                .Stringify())) {
      makefile->Target(
          source_file
              ->ResolveFullQualifiedObjectPath(working_folder, build_type)
              .Stringify(),
          ranges::views::single("jk_force"),
          ranges::views::empty<core::builder::CustomCommandLine>);
    }

    all_objects.push_back(
        source_file->ResolveFullQualifiedObjectPath(working_folder, build_type)
            .Stringify());
  }

  return all_objects;
}

void CCLibraryCompiler::generate_build_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    rules::CCLibrary *rule) const {
  (void)scc;
  auto makefile = new_makefile_with_common_commands(session, working_folder);

  makefile.Comment("Headers: ", absl::StrJoin(rule->ExpandedHeaderFiles, ", "));
  makefile.Comment("Sources: ", absl::StrJoin(rule->ExpandedSourceFiles, ", "));
  makefile.Comment("ACF: ",
                   absl::StrJoin(rule->ExpandedAlwaysCompileFiles, ", "));

  core::builder::CustomCommandLines clean_statements;

  // lint headers
  std::vector<std::string> lint_header_targets =
      lint_headers(session, working_folder, rule, &makefile);

  auto library_progress_num = rule->Steps.Step(".library");

  auto source_files =
      rule->ExpandedSourceFiles |
      ranges::views::transform([rule](const auto &filename) {
        return std::make_unique<models::cc::SourceFile>(filename, rule);
      }) |
      ranges::to_vector;

  for (const auto &build_type : session->BuildTypes) {
    auto all_objects = add_source_files_commands(
        session, working_folder, rule, &makefile, &lint_header_targets,
        source_files, build_type);

    auto library_file = working_folder.Sub(build_type, rule->LibraryFileName);
    makefile.Target(library_file.Stringify(), ranges::views::empty<std::string>,
                    ranges::views::empty<core::builder::CustomCommandLine>);

    auto clean_old_library = core::builder::CustomCommandLine::Make({
        "@$(RM)",
        library_file.Stringify(),
    });

    auto ar_stmt = core::builder::CustomCommandLine::Make({
        "@$(AR)",
        library_file.Stringify(),
    });
    std::copy(std::begin(all_objects), std::end(all_objects),
              std::back_inserter(ar_stmt));

    auto print_stmt = core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         fmt::format("--progress-num={}",
                     absl::StrJoin(rule->Steps.Steps(), ",")),
         fmt::format("--progress-dir={}",
                     session->Project->BuildRoot.Stringify()),
         fmt::format("Linking CXX static library {}",
                     library_file.Stringify())});

    makefile.Target(
        library_file.Stringify(),
        ranges::views::concat(
            ranges::views::all(all_objects),
            ranges::views::all(lint_header_targets),
            ranges::views::single(working_folder.Sub("build.make").Stringify()),
            ranges::views::single(
                working_folder.Sub("toolchain.make").Stringify()),
            ranges::views::single(
                working_folder.Sub("flags.make").Stringify())),
        ranges::views::concat(
            ranges::views::single(print_stmt),
            ranges::views::single(core::builder::CustomCommandLine::Make(
                {"@$(MKDIR)", library_file.Path.parent_path().string()})),
            ranges::views::single(clean_old_library),
            ranges::views::single(ar_stmt)));

    auto gen_rm = [](const auto &str) {
      return core::builder::CustomCommandLine::Make({"@$(RM)", str});
    };

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"@$(RM)", library_file.Stringify()}));
    std::transform(std::begin(all_objects), std::end(all_objects),
                   std::back_inserter(clean_statements), gen_rm);
    std::transform(std::begin(lint_header_targets),
                   std::end(lint_header_targets),
                   std::back_inserter(clean_statements), gen_rm);

    auto build_target = working_folder.Sub(build_type, "build").Stringify();
    makefile.Target(build_target,
                    ranges::views::single(library_file.Stringify()),
                    ranges::views::empty<core::builder::CustomCommandLine>,
                    "Rule to build all files generated by this target.", true);

    makefile.Target(build_type, ranges::views::single(build_target),
                    ranges::views::empty<core::builder::CustomCommandLine>,
                    "Rule to build all files generated by this target.", true);
  }

  makefile.Target("clean", ranges::views::empty<std::string>,
                  ranges::views::all(clean_statements), "", true);

  end_of_generate_build_file(&makefile, session, working_folder, rule);
}

}  // namespace jk::impls::compilers::makefile
