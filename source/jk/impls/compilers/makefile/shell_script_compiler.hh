// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/core/interfaces/compiler.hh"

namespace jk::impls::compilers::makefile {

struct ShellScriptCompiler : public core::interfaces::Compiler {
  std::string_view Name() const override;

  void Compile(
      core::models::Session *session,
      const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
      core::models::BuildRule *rule) const override;
};

}  // namespace jk::impls::compilers::makefile
