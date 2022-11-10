// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/impls/rules/cc_library.hh"

namespace jk::impls::rules {

class CCBinary : public CCLibrary {
 public:
  CCBinary(core::models::BuildPackage *package, utils::Kwargs kwargs,
           std::string type_name  = "cc_binary",
           core::models::RuleType = core::models::RuleType{
               core::models::RuleTypeEnum::kBinary,
               core::models::RuleTypeEnum::kCC,
           });

  const std::vector<std::string> &ExportedFiles(
      core::models::Session *session, std::string_view build_type) override;

 private:
  std::vector<std::string> _binary_tmp_file;
};

}  // namespace jk::impls::rules
