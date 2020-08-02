// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/compiler/shell_script.hh"

#include <string>

#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/package.hh"
#include "jk/rules/external/rules/shell_script.hh"

namespace jk::rules::external {

static const char *ExternalInstalledPrefix = ".build/.lib/m${PLATFORM}";

std::string MakefileShellScriptCompiler::Name() const {
  return "Makefile.external_project";
}

void MakefileShellScriptCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<ShellScript>();
  core::output::UnixMakefilePtr makefile{
      new core::output::UnixMakefile{"build.make"}};
  makefile->DefineCommon(project);

  makefile->DefineEnvironment("MKDIR", "mkdir -p");

  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  makefile->AddTarget("all", {}, {}, "", true);

  makefile->AddTarget("jk_force", {}, {}, "This target is always outdated.",
                      true);

  auto script_target = working_folder.Sub("build").Stringify();
  auto print_stmt = core::builder::CustomCommandLine::Make({
      "@$(PRINT)",
      "--switch=$(COLOR)",
      "--green",
      "--bold",
      "--progress-num={}"_format(common::Counter()->Next()),
      "--progress-dir={}"_format(project->BuildRoot),
      "Installing External Project {}"_format(rule->FullQualifiedName()),
  });

  auto mkdir_stmt = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)",
       project->ProjectRoot.Sub(ExternalInstalledPrefix).Stringify()});

  auto run_stmt = core::builder::CustomCommandLine::Make(
      {"@{}/{}"_format(project->Resolve(rule->Package->Path), rule->Script),
       "{}"_format(common::FLAGS_platform == common::Platform::k32 ? 32 : 64)});

  makefile->AddTarget(script_target, {"jk_force"},
                      core::builder::CustomCommandLines::Multiple(
                          print_stmt, mkdir_stmt, run_stmt));

  makefile->AddTarget("build", {script_target}, {}, "", true);

  auto w = wf->Build(working_folder.Sub("build.make").Stringify());
  makefile->Write(w.get());
}

std::string CompileDatabaseShellScriptCompiler::Name() const {
  return "CompileDatabase.external_project";
}

void CompileDatabaseShellScriptCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  (void)project;
  (void)wf;
  (void)rule;
  (void)expander;
}

}  // namespace jk::rules::external

// vim: fdm=marker
