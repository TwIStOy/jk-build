// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/rules/cc/compiler/cc_binary.hh"

namespace jk::rules::cc {

struct MakefileCCTestCompiler : MakefileCCBinaryCompiler {
  std::string Name() const override;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
