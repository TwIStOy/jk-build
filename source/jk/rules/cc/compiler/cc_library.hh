// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/source_file.hh"

namespace jk::rules::cc {

extern std::vector<std::string> BuildTypes;

struct MakefileCCLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

  core::output::UnixMakefilePtr GenerateFlags(core::writer::Writer *w,
                                              CCLibrary *rule) const;

  core::output::UnixMakefilePtr GenerateToolchain(
      core::writer::Writer *w) const;

  core::output::UnixMakefilePtr GenerateBuild(
      core::filesystem::ProjectFileSystem *project,
      const common::AbsolutePath &working_folder, core::writer::Writer *w,
      CCLibrary *rule,
      core::filesystem::FileNamePatternExpander *expander) const;

  //! return `ProgressNum`
  uint32_t LintSourceFile(core::filesystem::ProjectFileSystem *project,
                          SourceFile *source_file,
                          core::output::UnixMakefile *build,
                          const common::AbsolutePath &working_folder) const;

  void MakeSourceFile(core::filesystem::ProjectFileSystem *project,
                      const std::string &build_type, SourceFile *source_file,
                      const std::list<std::string> &headers,
                      core::output::UnixMakefile *build,
                      const common::AbsolutePath &working_folder) const;
};

struct CompileDatabaseCCLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
