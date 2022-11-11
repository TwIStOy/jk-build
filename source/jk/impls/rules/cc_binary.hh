// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "absl/container/flat_hash_map.h"
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

  absl::flat_hash_map<std::string, std::vector<std::string> >
      RawAddtionalDependencies;
  struct DependentInBinary {
    core::models::BuildRule *Rule;
    std::vector<core::models::BuildRule *> Dependencies;
  };
  std::vector<DependentInBinary> DependenciesInBinary;

 protected:
  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

 private:
  std::vector<std::string> _binary_tmp_file;
};

}  // namespace jk::impls::rules
