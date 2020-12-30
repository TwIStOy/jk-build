// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/rules/cc/compiler/cc_library.hh"

namespace jk::rules::cc {

struct MakefileCCBinaryCompiler : MakefileCCLibraryCompiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
      core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

  core::output::UnixMakefilePtr GenerateBuild(
      core::filesystem::JKProject *project,
      const common::AbsolutePath &working_folder, core::writer::Writer *w,
      CCLibrary *rule,
      core::filesystem::FileNamePatternExpander *expander) const;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
