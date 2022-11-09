// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/core/models/build_rule.hh"

namespace jk::impls::rules {

class ShellScript : public core::models::BuildRule {
 public:
  ShellScript(core::models::BuildPackage *package, utils::Kwargs kwargs,
              std::string type_name  = "shell_script",
              core::models::RuleType = core::models::RuleType{
                  core::models::RuleTypeEnum::kExternal,
              });

  void DoPrepare(core::models::Session *session) override;

  std::string Script;
  std::vector<std::string> Exports;
  std::vector<std::string> LdFlags;
  std::unordered_map<std::string, std::string> ExportBin;

  const std::vector<std::string> &ExportedFiles(
      core::models::Session *session, std::string_view build_type) override;

 protected:
  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;
};

}  // namespace jk::impls::rules
