// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/impls/rules/cc_binary.hh"

namespace jk::impls::rules {

class CCTest : public CCBinary {
 public:
  CCTest(core::models::BuildPackage *package, utils::Kwargs kwargs,
         std::string type_name  = "cc_test",
         core::models::RuleType = core::models::RuleType{
             core::models::RuleTypeEnum::kBinary,
             core::models::RuleTypeEnum::kCC,
             core::models::RuleTypeEnum::kTest,
         });
};

}  // namespace jk::impls::cc
