// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_binary.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/lang/cc/rules/cc_test.hh"
#include "jk/lang/cc/source_file.hh"

namespace jk::lang::cc {

struct MakefileCCLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

 protected:
  void GenerateFlags(core::writer::Writer *w,
                     core::rules::CCLibrary *rule) const;

  void GenerateToolchain(core::writer::Writer *w) const;

  void GenerateBuild(core::filesystem::ProjectFileSystem *project,
                     const common::AbsolutePath &working_folder,
                     core::writer::Writer *w, core::rules::CCLibrary *rule,
                     core::filesystem::FileNamePatternExpander *expander) const;

  void MakeSourceFile(core::filesystem::ProjectFileSystem *project,
                      SourceFile *source_file, uint32_t idx,
                      uint32_t source_files_count,
                      const std::list<std::string> &headers,
                      core::output::UnixMakefile *build,
                      const common::AbsolutePath &working_folder) const;
};

}  // namespace jk::lang::cc

// vim: fdm=marker