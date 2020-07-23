// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "jk/common/path.hh"
#include "jk/utils/str.hh"

namespace jk {

namespace core::rules {
struct BuildPackage;
struct BuildRule;
}  // namespace core::rules

namespace lang::cc {

/// SourceFile => a cpp filename
struct SourceFile : public utils::Stringifiable {
  static SourceFile *Create(core::rules::BuildRule *rule,
                            core::rules::BuildPackage *package,
                            std::string filename);

  core::rules::BuildRule *Rule;
  core::rules::BuildPackage *Package;

  // FileName written in 'BUILD'
  std::string FileName;

  common::ProjectRelativePath FullQualifiedPath() const;

  common::AbsolutePath FullQualifiedObjectPath(
      const common::AbsolutePath &new_root,
      const std::string &build_type) const;

  common::ProjectRelativePath FullQualifiedObjectPath() const;

  common::AbsolutePath FullQualifiedLintPath(
      const common::AbsolutePath &new_root) const;

  bool IsCppSourceFile() const;
  bool IsCSourceFile() const;
  bool IsSourceFile() const;

  std::string Stringify() const final;

  uint32_t ProgressNum;

 private:
  SourceFile(core::rules::BuildRule *rule, core::rules::BuildPackage *package,
             std::string filename);

 private:
  static std::unordered_map<std::string, std::unique_ptr<SourceFile>>
      source_files_;
};

}  // namespace lang::cc
}  // namespace jk
