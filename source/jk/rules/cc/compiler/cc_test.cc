// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_test.hh"

#include <string>

namespace jk::rules::cc {

std::string MakefileCCTestCompiler::Name() const {
  return "Makefile.cc_test";
}

}  // namespace jk::rules::cc

// vim: fdm=marker

