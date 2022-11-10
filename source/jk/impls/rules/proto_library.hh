// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/impls/rules/cc_library.hh"

namespace jk::impls::rules {

class ProtoLibrary : public CCLibrary {
 public:
  ProtoLibrary(core::models::BuildPackage *package, utils::Kwargs kwargs,
               std::string type_name  = "proto_library",
               core::models::RuleType = core::models::RuleType{
                   core::models::RuleTypeEnum::kLibrary,
                   core::models::RuleTypeEnum::kCC,
               });

  void DoPrepare(core::models::Session *session) override;
};

}  // namespace jk::impls::rules
