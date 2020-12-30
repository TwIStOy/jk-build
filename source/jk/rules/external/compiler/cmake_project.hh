// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/core/compile/compile.hh"
#include "jk/rules/external/compiler/external_library.hh"

namespace jk::rules::external {

struct MakefileCMakeLibrary : public MakefileExternalLibraryCompiler {
  std::string Name() const override;

  void Compile(core::filesystem::JKProject *project,
               core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
               core::filesystem::FileNamePatternExpander *expander =
                   &core::filesystem::kDefaultPatternExpander) const override;
};

}  // namespace jk::rules::external

// vim: fdm=marker
