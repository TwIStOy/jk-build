// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <vector>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk::rules::external {

/*
external_project(
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

  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::ProjectFileSystem *project,
      const std::string &build_type) const override;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::vector<std::string> ExportedHeaders() const override;

  // --- Fields Start ---
  std::string Script;
  std::vector<std::string> Exports;
  std::vector<std::string> LdFlags;
  std::vector<std::string> Headers;
  // --- Fields End ---
};

}  // namespace jk::rules::external

// vim: fdm=marker
