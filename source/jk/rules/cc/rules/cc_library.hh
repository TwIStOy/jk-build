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

/// cpp static library
class CCLibrary : public BuildRule {
 public:
  CCLibrary(BuildPackage *package, std::string name,
            std::initializer_list<RuleTypeEnum> types = {RuleTypeEnum::kLibrary,
                                                         RuleTypeEnum::kCC},
            std::string_view type_name = "cc_library",
            std::string exported_file_name = "");

  bool IsStable() const override;

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedFilesSimpleName(
      core::filesystem::ProjectFileSystem *project,
      const std::string &build_type) const override;

  //! Get all **includes** recursively
  const std::vector<std::string> &ResolveIncludes() const;

  //! Get all **definitions** recursively
  const std::vector<std::string> &ResolveDefinitions() const;

  //! All compile flags for cpp files.
  const std::vector<std::string> &FlagsForCppFiles() const;

  //! All compile flags for c files.
  const std::vector<std::string> &FlagsForCFiles() const;

  //! Expand source files.
  const std::vector<std::string> &ExpandSourceFiles(
      core::filesystem::ProjectFileSystem *project,
      core::filesystem::FileNamePatternExpander *expander) const;

  std::vector<std::string> ExportedLinkFlags() const override;

  std::vector<std::string> ExportedHeaders() const override;

  // --- Fields Start ---
  std::vector<std::string> CFlags;
  std::vector<std::string> CppFlags;
  std::vector<std::string> CxxFlags;
  std::vector<std::string> LdFlags;  // unused
  std::vector<std::string> Sources;
  std::vector<std::string> Excludes;
  std::vector<std::string> Includes;
  std::vector<std::string> Defines;
  std::vector<std::string> Headers;
  // --- Fields End ---

  const std::string ExportedFileName;

 private:
  mutable boost::optional<std::vector<std::string>> resolved_includes_;
  mutable boost::optional<std::vector<std::string>> resolved_definitions_;
  mutable boost::optional<std::vector<std::string>> resolved_c_flags_;
  mutable boost::optional<std::vector<std::string>> resolved_cpp_flags_;
  mutable boost::optional<std::vector<std::string>> expanded_source_files_;
};

}  // namespace jk::rules::cc
