// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/lang/cc/cc_library.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace rules {

class CCBinary : public CCLibrary {
 public:
  CCBinary(BuildPackage *package, std::string name,
           std::initializer_list<RuleTypeEnum> types = {RuleTypeEnum::kBinary})
      : CCLibrary(package, name, types) {
  }
};

}  // namespace rules
}  // namespace core
}  // namespace jk

