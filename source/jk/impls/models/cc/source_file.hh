// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <string_view>

#include "jk/common/path.hh"

namespace jk::core::models {

class BuildRule;
class BuildPackage;

}  // namespace jk::core::models

namespace jk::impls::models::cc {

struct SourceFile {
  explicit SourceFile(std::string FileName, core::models::BuildRule *Rule);
  explicit SourceFile(const common::ProjectRelativePath &FileName,
                      core::models::BuildRule *Rule);

  core::models::BuildRule *Rule;

  bool lint{false};

  bool IsCSourceFile;
  bool IsCppSourceFile;
  bool IsSourceFile;
  bool IsHeaderFile;

  common::ProjectRelativePath FullQualifiedPath;
  common::ProjectRelativePath FullQualifiedObjectPath;

  common::AbsolutePath ResolveFullQualifiedPath(
      const common::AbsolutePath &new_root) const;

  common::AbsolutePath ResolveFullQualifiedObjectPath(
      const common::AbsolutePath &new_root, std::string_view build_type) const;

  common::AbsolutePath ResolveFullQualifiedDotDPath(
      const common::AbsolutePath &new_root) const;

  common::AbsolutePath ResolveFullQualifiedLintPath(
      const common::AbsolutePath &new_root) const;

  common::AbsolutePath ResolveFullQualifiedPbPath(
      const common::AbsolutePath &new_root) const;
};

}  // namespace jk::impls::models::cc
