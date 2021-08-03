// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <string_view>
#include <vector>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk::rules::external {

/**
 * external_library(
 *   name = "name",
 *   url = "",
 *   sha256 = "",
 *   type = "tar.gz" | "zip" | "tar",
 *   download_command = [],
 *   configure_command = [],
 *   build_command = [],
 *   install_command = [],
 *   libraries = [], // from ${JK_BUNDLE_LIBRARY_PREFIX}
 *   ldflags = [],
 *   version = "", // VERSION STRING
 * )
 */
class ExternalLibrary : public core::rules::BuildRule {
 public:
  ExternalLibrary(core::rules::BuildPackage *package, std::string name,
                  std::string_view rule_type_name = "external_library");

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::JKProject *project,
      const std::string &build_type) const override;

  std::vector<std::string> ExportedLinkFlags() const override;

  // --- Fields Start ---
  std::string Url;
  std::string Sha256;
  std::string ArchiveType;
  std::vector<std::string> DownloadCommand;
  std::vector<std::string> ConfigureCommand;
  std::vector<std::string> BuildCommand;
  std::vector<std::string> InstallCommand;
  std::vector<std::string> LdFlags;
  std::vector<std::string> Libraries;
  // ---- Fields End ----

 private:
};

}  // namespace jk::rules::external

// vim: fdm=marker
