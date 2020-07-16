// Copyright (c) 2020 Hawtian Wang
//

#include "test/jk/core/compile/nop_expander.hh"

namespace jk::test {

std::list<std::string> NopExpander::Expand(const std::string &pattern,
                                           const common::AbsolutePath &) {
  return std::list<std::string>{pattern};
}

}  // namespace jk::test

// vim: fdm=marker

