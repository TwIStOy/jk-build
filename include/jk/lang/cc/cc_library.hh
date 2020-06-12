// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <initializer_list>
#include <list>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace rules {

/// cpp static library
class CCLibrary : public BuildRule {
 public:
  CCLibrary(BuildPackage *package, std::string name,
            std::initializer_list<RuleTypeEnum> types = {
                RuleTypeEnum::kLibrary});

  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  // --- Fields Start ---
  std::vector<std::string> CFlags;
  std::vector<std::string> CppFlags;
  std::vector<std::string> CxxFlags;
  std::vector<std::string> LdFlags;
  std::vector<std::string> Sources;
  std::vector<std::string> Excludes;
  std::vector<std::string> Includes;
  std::vector<std::string> Defines;
  std::vector<std::string> Headers;
  // --- Fields End ---

  const std::string ExportedFileName;
};

}  // namespace rules
}  // namespace core
}  // namespace jk

