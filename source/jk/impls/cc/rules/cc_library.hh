// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "jk/common/path.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/session.hh"

namespace jk::impls::cc {

class CCLibrary : public core::models::BuildRule {
 public:
  void Prepare(core::models::Session *session) override;

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

  common::AbsolutePath package_root_;
  absl::flat_hash_set<std::string> excludes_;

 public:
  absl::flat_hash_set<std::string> NolintFiles;
  std::vector<std::string> ExpandedHeaderFiles;
  std::vector<std::string> ExpandedSourceFiles;
  std::vector<std::string> ExpandedAlwaysCompileFiles;
  std::string LibraryFileName;
  std::vector<std::string> CFileFlags;
  std::vector<std::string> CppFileFlags;
  absl::flat_hash_set<std::string> PlainIncludeFlags;

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
