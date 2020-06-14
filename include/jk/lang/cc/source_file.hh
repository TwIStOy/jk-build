// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

namespace jk {

namespace core::rules {
struct BuildPackage;
struct BuildRule;
}  // namespace core::rules

namespace lang::cc {

/// SourceFile => a cpp filename
struct SourceFile {
  core::rules::BuildRule *Rule;
  core::rules::BuildPackage *Package;

  // FileName written in 'BUILD'
  std::string FileName;

  std::string FullQualifiedName() const;
  std::string FullQualifiedObjectName() const;
};

}  // namespace lang::cc
}  // namespace jk
