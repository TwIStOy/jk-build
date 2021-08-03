// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk::rules::external {

/*
shell_script(
  name = "glog",
  script = "install_glog.py",
  export = [],
  ldflags = [],
  deps = [],
)
 */
class ShellScript : public core::rules::BuildRule {
 public:
  ShellScript(core::rules::BuildPackage *package, std::string name);

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::JKProject *project,
      const std::string &build_type) const override;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::unordered_map<std::string, std::string> ExportedEnvironmentVar(
      core::filesystem::JKProject *) const override;

  // --- Fields Start ---
  std::string Script;
  std::vector<std::string> Exports;
  std::vector<std::string> LdFlags;
  std::unordered_map<std::string, std::string> ExportBin;
  // --- Fields End ---
};

}  // namespace jk::rules::external

// vim: fdm=marker
