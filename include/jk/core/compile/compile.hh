// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <unordered_map>

#include "jk/core/rules/build_rule.hh"
#include "jk/lang/cc/cc_library.hh"

namespace jk::core::compile {

class Compiler {
 public:
  void Compile(rules::BuildRule *rule);

 private:
  void Compile_cc_library(rules::CCLibrary *rule);
};

}  // namespace jk::core::compile

