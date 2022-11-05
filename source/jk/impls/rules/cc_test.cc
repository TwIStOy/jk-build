// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_test.hh"

namespace jk::impls::rules {

CCTest::CCTest(core::models::BuildPackage *package, utils::Kwargs kwargs,
               std::string type_name, core::models::RuleType type)
    : CCBinary(package, std::move(kwargs), std::move(type_name), type) {
}

}  // namespace jk::impls::rules
