// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string_view>

#include "jk/core/interfaces/compiler.hh"

namespace jk::impls::compilers {

struct NopCompiler : core::interfaces::Compiler {
  std::string_view Name() const final;

  void Compile(
      core::models::Session *session,
      const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
      core::models::BuildRule *rule) const final;
};

}  // namespace jk::impls::compilers
