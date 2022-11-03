// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>

#include "jk/core/models/build_rule_base.hh"

namespace jk::core::models {

class BuildRule {
 public:
  std::unique_ptr<BuildRuleBase> Base;

 private:
};

}  // namespace jk::core::models
