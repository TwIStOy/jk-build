// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/project.hh"

namespace jk::external {

struct MakefileExternalProjectCompiler final : public core::compile::Compiler {
  std::string Name() const;

  void Compile(core::filesystem::ProjectFileSystem *project,
               core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
               core::filesystem::FileNamePatternExpander *expander =
                   &core::filesystem::kDefaultPatternExpander) const;
};

struct CompileDatabaseExternalProjectCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;
};

}  // namespace jk::external

// vim: fdm=marker

