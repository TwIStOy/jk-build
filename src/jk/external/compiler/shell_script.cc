// Copyright (c) 2020 Hawtian Wang
//

#include "jk/external/compiler/shell_script.hh"

#include <string>

#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/writer/writer.hh"

namespace jk::external {

std::string MakefileShellScriptCompiler::Name() const {
  return "Makefile.shell_script";
}

void MakefileShellScriptCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  core::output::UnixMakefilePtr makefile{new core::output::UnixMakefile{
      working_folder.Sub("build.make").Stringify()}};

  // rule->ExportedFilesSimpleName();
  //
  // rule->FullQualifiedName();
  //
  // makefile->AddTarget(std::string target, std::list<std::string> deps);
}

}  // namespace jk::external

// vim: fdm=marker

