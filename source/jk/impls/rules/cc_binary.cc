// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_binary.hh"

namespace jk::impls::rules {

CCBinary::CCBinary(core::models::BuildPackage *package, utils::Kwargs kwargs,
                   std::string type_name, core::models::RuleType type)
    : CCLibrary(package, std::move(kwargs), std::move(type_name), type) {
}

}  // namespace jk::impls::cc
