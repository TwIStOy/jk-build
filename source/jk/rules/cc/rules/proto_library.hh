// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <initializer_list>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "pybind11/pytypes.h"

namespace jk::rules::cc {

using core::rules::BuildPackage;
using core::rules::BuildRule;
using core::rules::RuleTypeEnum;

class ProtoLibrary : public BuildRule {
 public:
  ProtoLibrary(BuildPackage *package, std::string name);

  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::vector<std::string> ExportedHeaders() const override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::ProjectFileSystem *project,
      const std::string &build_type) const override;

  // --- Fields Start ---
  std::vector<std::string> Sources;
  // --- Fields End ---

  const std::string ExportedFileName;
};

}  // namespace jk::rules::cc

// vim: fdm=marker

