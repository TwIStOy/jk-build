// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_package.hh"

#include "jk/core/models/build_rule_factory.hh"

namespace jk::core::models {

void BuildPackage::ConstructRules(
    std::vector<core::executor::ScriptInterpreter::EvalResult> raw_eval_results,
    core::models::BuildRuleFactory *factory) {
  for (auto &&r : raw_eval_results) {
    auto rule     = factory->Create(r.FuncName, this, std::move(r.Args));
    rule->Package = this;
    RulesMap.emplace(*rule->Base->Name, std::move(rule));
  }
}

}  // namespace jk::core::models
