// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_binary.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <string>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

static auto logger = utils::Logger("compiler.cc_binary");

std::string MakefileCCBinaryCompiler::Name() const {
  return "Makefile.cc_binary";
}

void MakefileCCBinaryCompiler::Compile(
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CCBinary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

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

static std::vector<std::string> DEBUG_LDFLAGS_BEFORE = {
    "-ftest-coverage",
    "-fprofile-arcs",
};

core::output::UnixMakefilePtr MakefileCCBinaryCompiler::GenerateBuild(
    core::filesystem::JKProject *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    CCLibrary *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  logger->debug("binary actions");

  auto rule = _rule->Downcast<CCBinary>();
  core::output::UnixMakefilePtr build(
      new core::output::UnixMakefile{"build.make"});

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  // lint files first
  for (const auto &filename : source_files) {
    auto source_file = SourceFile::Create(rule, rule->Package, filename);

    LintSourceFile(project, rule, source_file, build.get(), working_folder);
  }

  // lint headers
  const auto &header_files = rule->ExpandedHeaderFiles(project, expander);
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

  auto binary_progress_num = rule->KeyNumber(".binary");
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
    auto binary_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    auto deps_and_flags =
        rule->ResolveDependenciesAndLdFlags(project, build_type);

    auto binary_deps = all_objects;
    binary_deps.insert(binary_deps.end(), std::begin(lint_header_targets),
                       std::end(lint_header_targets));
    // depend on all static libraries
    for (auto dep : rule->DependenciesAlwaysBehind()) {
      if (!dep->Type.IsExternal() && dep != rule) {
        auto names = dep->ExportedFilesSimpleName(project, build_type);
        std::copy(std::begin(names), std::end(names),
                  std::back_inserter(binary_deps));
      }
    }
    // depend on my 'build.make'
    binary_deps.push_back(working_folder.Sub("build.make"));

    auto mkdir_stmt = core::builder::CustomCommandLine::Make(
        {"@$(MKDIR)", binary_file.Path.parent_path().string()});

    auto print_stmt = core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         "--progress-num={}"_format(utils::JoinString(",", rule->KeyNumbers())),
         "--progress-dir={}"_format(project->BuildRoot),
         "Linking binary {}"_format(binary_file.Stringify())});

    auto link_stmt = core::builder::CustomCommandLine::Make({"@$(LINKER)"});
    std::copy(std::begin(all_objects), std::end(all_objects),
              std::back_inserter(link_stmt));
    std::copy(std::begin(deps_and_flags), std::end(deps_and_flags),
              std::back_inserter(link_stmt));
    link_stmt.push_back("-g");
    link_stmt.push_back("${}{}_LDFLAGS{}"_format("{", build_type, "}"));
    link_stmt.push_back("-o");
    link_stmt.push_back(binary_file.Stringify());

    build->AddTarget(binary_file.Stringify(), binary_deps,
                     core::builder::CustomCommandLines::Multiple(
                         print_stmt, mkdir_stmt, link_stmt));
    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"@$(RM)", "{}"_format(binary_file.Stringify())}));
    for (const auto &obj : all_objects) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"@$(RM)", obj}));
    }
    for (const auto &obj : lint_header_targets) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"@$(RM)", obj}));
    }

    auto build_target = working_folder.Sub(build_type).Sub("build").Stringify();
    build->AddTarget(build_target, {binary_file.Stringify()}, {},
                     "Rule to build all files generated by this target.", true);

    build->AddTarget(build_type, {build_target}, {}, "", true);
  }

  build->AddTarget("clean", {}, clean_statements, "", true);

  AddtionalAction(build.get(), working_folder, rule);

  build->Write(w);

  return build;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
