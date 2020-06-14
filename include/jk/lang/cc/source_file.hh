// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace jk {

namespace core::rules {
struct BuildPackage;
struct BuildRule;
}  // namespace core::rules

namespace lang::cc {

/// SourceFile => a cpp filename
struct SourceFile {
  SourceFile(core::rules::BuildRule *rule, core::rules::BuildPackage *package,
             std::string filename);

  core::rules::BuildRule *Rule;
  core::rules::BuildPackage *Package;

  // FileName written in 'BUILD'
  std::string FileName;

  std::string FullQualifiedName() const;
  std::string FullQualifiedObjectName() const;

 private:
  static std::unordered_map<std::string, std::unique_ptr<SourceFile>>
      source_files_;
};

}  // namespace lang::cc
}  // namespace jk
