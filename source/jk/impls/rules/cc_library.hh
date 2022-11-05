// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "jk/common/path.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/session.hh"
#include "jk/utils/kwargs.hh"

namespace jk::impls::rules {

class CCLibrary : public core::models::BuildRule {
 public:
  CCLibrary(core::models::BuildPackage *package, utils::Kwargs kwargs,
            std::string type_name  = "cc_library",
            core::models::RuleType = core::models::RuleType{
                core::models::RuleTypeEnum::kLibrary,
                core::models::RuleTypeEnum::kCC,
            });

  void DoPrepare(core::models::Session *session) override;

  std::vector<std::string> CFlags;
  std::vector<std::string> CppFlags;
  std::vector<std::string> CxxFlags;
  std::vector<std::string> LdFlags;  // unused
  std::vector<std::string> Sources;
  std::vector<std::string> Excludes;
  std::vector<std::string> Includes;
  std::vector<std::string> Defines;
  std::vector<std::string> Headers;
  std::vector<std::string> AlwaysCompile;

 protected:
  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

 private:
  void prepare_nolint_files(core::models::Session *session);
  void prepare_excludes(core::models::Session *session);
  void prepare_source_files(core::models::Session *session);
  void prepare_header_files(core::models::Session *session);
  void prepare_always_compile_files(core::models::Session *session);
  void prepare_include_flags(core::models::Session *session);
  void prepare_define_flags(core::models::Session *session);

  std::optional<common::AbsolutePath> package_root_;
  absl::flat_hash_set<std::string> excludes_;

 public:
  absl::flat_hash_set<std::string> NolintFiles;
  std::vector<std::string> ExpandedHeaderFiles;
  std::vector<std::string> ExpandedSourceFiles;
  std::vector<std::string> ExpandedAlwaysCompileFiles;
  std::string LibraryFileName;
  std::vector<std::string> ExpandedCFileFlags;
  std::vector<std::string> ExpandedCppFileFlags;
  absl::flat_hash_set<std::string> ResolvedIncludes;
  absl::flat_hash_set<std::string> ResolvedDefines;

  template<typename T>
  bool InNoLint(T &&name) const;
};

template<typename T>
auto CCLibrary::InNoLint(T &&name) const -> bool {
  if (NolintFiles.empty()) {
    return false;
  }

  return NolintFiles.contains(std::forward<T>(name));
}

}  // namespace jk::impls::cc
