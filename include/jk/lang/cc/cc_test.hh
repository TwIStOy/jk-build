// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/lang/cc/cc_binary.hh"
#include "jk/lang/cc/cc_library.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace rules {

class CCTest : public CCBinary {
 public:
  CCTest(BuildPackage *package, std::string name)
      : CCBinary(package, name, {RuleTypeEnum::kBinary, RuleTypeEnum::kTest},
                 "cc_test") {
  }
};

}  // namespace rules
}  // namespace core
}  // namespace jk

