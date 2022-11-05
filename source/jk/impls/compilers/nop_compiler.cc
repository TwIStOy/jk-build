// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/nop_compiler.hh"

namespace jk::impls::compilers {

NopCompiler::NopCompiler(std::string_view str) : str_(str) {
}

auto NopCompiler::Name() const -> std::string_view {
  return str_;
}

auto NopCompiler::Compile(core::models::Session *session,
                          core::models::BuildRule *rule) const -> void {
  (void)session;
  (void)rule;
  // do nothing
}

}  // namespace jk::impls::compilers
