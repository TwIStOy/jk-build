// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "pybind11/pytypes.h"

namespace jk::rules::cc {

class CCBinary : public CCLibrary {
 public:
  CCBinary(BuildPackage *package, std::string name,
           std::initializer_list<RuleTypeEnum> types = {RuleTypeEnum::kBinary,
                                                        RuleTypeEnum::kCC},
           std::string_view type_name = "cc_binary")
      : CCLibrary(package, name, types, type_name, name) {
  }

  std::vector<std::string> ResolveDependenciesAndLdFlags(
      core::filesystem::JKProject *project,
      const std::string &build_type) const;
};

}  // namespace jk::rules::cc
