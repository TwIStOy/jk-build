// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/common/path.hh"
#include "jk/core/executor/script.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/build_rule_factory.hh"
#include "jk/utils/cpp_features.hh"
#include "range/v3/view/map.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::core::models {

class __JK_HIDDEN BuildPackage {
 public:
  std::string Name;
  common::ProjectRelativePath Path;

  auto IterRules() {
    return RulesMap | ranges::views::values |
           ranges::views::transform([](auto &x) {
             return x.get();
           });
  }

  void ConstructRules(std::vector<core::executor::ScriptInterpreter::EvalResult>
                          raw_eval_results,
                      core::models::BuildRuleFactory *factory);

  absl::flat_hash_map<std::string, std::unique_ptr<BuildRule>> RulesMap;
};

}  // namespace jk::core::models
