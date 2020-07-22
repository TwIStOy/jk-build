// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/lang/cc/compiler/cc_binary.hh"

namespace jk::lang::cc {

struct MakefileCCTestCompiler : MakefileCCBinaryCompiler {
  std::string Name() const override;
};

}  // namespace jk::lang::cc

// vim: fdm=marker

