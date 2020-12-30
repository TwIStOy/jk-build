// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/rules/external/rules/external_library.hh"

namespace jk::rules::external {

struct MakefileExternalLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(core::filesystem::JKProject *project,
               core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
               core::filesystem::FileNamePatternExpander *expander =
                   &core::filesystem::kDefaultPatternExpander) const override;

  void CompileImpl(core::filesystem::JKProject *project,
                   core::writer::WriterFactory *wf, ExternalLibrary *rule,
                   std::function<void(core::output::UnixMakefile *)> prepare,
                   const std::vector<std::string> &download_command,
                   const std::vector<std::string> &configure_command,
                   const std::vector<std::string> &build_command,
                   const std::vector<std::string> &install_command) const;
};

}  // namespace jk::rules::external

// vim: fdm=marker
