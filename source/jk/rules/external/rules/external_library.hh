// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <vector>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk::rules::external {

/**
 * external_library(
 *   name = "name",
 *   anchor = [],
 *   url = "",
 *   sha256 = "",
 *   type = "",
 *   header_only = true,
 * )
 */
class ExternalLibrary : public core::rules::BuildRule {
 public:
  ExternalLibrary(core::rules::BuildPackage *package, std::string name);

  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::ProjectFileSystem *project,
      const std::string &build_type) const override;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::vector<std::string> ExportedHeaders() const override;

  // --- Fields Start ---
  std::string Url;
  std::string Sha256;
  std::string ArchiveType;
  std::vector<std::string> Anchors;
  bool HeaderOnly;
  // ---- Fields End ----
};

}  // namespace jk::rules::external

// vim: fdm=marker
