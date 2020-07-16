// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/lang/cc/compiler/cc_library.hh"

namespace jk::lang::cc {

struct MakefileCCBinaryCompiler : MakefileCCLibraryCompiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

 private:
  void GenerateBuild(core::filesystem::ProjectFileSystem *project,
                     const common::AbsolutePath &working_folder,
                     core::writer::Writer *w, core::rules::CCLibrary *rule,
                     core::filesystem::FileNamePatternExpander *expander) const;
};

}  // namespace jk::lang::cc

// vim: fdm=marker

