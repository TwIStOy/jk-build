// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/core/interfaces/compiler.hh"

namespace jk::impls::compilers::makefile {

struct RootCompiler {
  std::string_view Name() const;

  void Compile(
      core::models::Session *session,
      const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
      std::vector<core::models::BuildRule *> rule) const;
};

}  // namespace jk::impls::compilers::makefile
