// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/nop_compiler.hh"

namespace jk::impls::compilers {

auto NopCompiler::Name() const -> std::string_view {
  return "nop";
}

auto NopCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    core::models::BuildRule *rule) const -> void {
  (void)session;
  (void)scc;
  (void)rule;
  // do nothing
}

}  // namespace jk::impls::compilers
