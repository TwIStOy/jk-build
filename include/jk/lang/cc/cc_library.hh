// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace rules {

class CCLibrary : public BuildRule {
  bool IsStable() const override;

  void ExtractFieldFromArguments(
      const std::unordered_map<std::string, pybind11::object>& kwargs) override;

  std::vector<std::string> CFlags;
  std::vector<std::string> CppFlags;
  std::vector<std::string> CxxFlags;
  std::vector<std::string> LdFlags;
  std::vector<std::string> Sources;
  std::vector<std::string> Excludes;
  std::vector<std::string> Includes;
  std::vector<std::string> Defines;
};

}  // namespace rules
}  // namespace core
}  // namespace jk

