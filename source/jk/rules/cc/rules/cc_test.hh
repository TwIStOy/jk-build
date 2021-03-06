// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "pybind11/pytypes.h"

namespace jk::rules::cc {

class CCTest : public CCBinary {
 public:
  CCTest(BuildPackage *package, std::string name)
      : CCBinary(
            package, name,
            {RuleTypeEnum::kBinary, RuleTypeEnum::kTest, RuleTypeEnum::kCC},
            "cc_test") {
  }
};

}  // namespace jk::rules::cc
