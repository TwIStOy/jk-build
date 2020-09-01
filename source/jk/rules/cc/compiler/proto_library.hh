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
#include "jk/rules/cc/rules/proto_library.hh"

namespace jk::rules::cc {

struct MakefileProtoLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;
};

}  // namespace jk::rules::cc

// vim: fdm=marker

