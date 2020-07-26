// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <vector>

#include "jk/core/rules/build_rule.hh"

namespace jk::external {

class ShellScript : public core::rules::BuildRule {
 public:
  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName() const override;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::vector<std::string> ExportedHeaders() const override;

  // --- Fields Start ---
  std::string Script;
  std::vector<std::string> Export;
  std::vector<std::string> LdFlags;
  std::vector<std::string> Headers;
  // --- Fields End ---

 private:
};

}  // namespace jk::external

// vim: fdm=marker

