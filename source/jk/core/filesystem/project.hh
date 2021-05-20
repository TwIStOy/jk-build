// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <optional>

#include "jk/common/path.hh"
#include "jk/core/filesystem/configuration.hh"
#include "toml.hpp"

namespace jk::core::filesystem {

enum class TargetPlatform {
  k32,
  k64,
};

struct JKProject {
  //! Returns the project from current working directory
  static JKProject ResolveFrom(const common::AbsolutePath &cwd);

  explicit JKProject(common::AbsolutePath ProjectRoot,
                     TargetPlatform Platform = TargetPlatform::k64,
                     std::optional<common::AbsolutePath> BuildRoot = {});

  //! Root path of this project. All packages path will be relative with this.
  const common::AbsolutePath ProjectRoot;

  //! Root path of build environment. Default is '.build' in **ProjectRoot**.
  const common::AbsolutePath BuildRoot;

  const TargetPlatform Platform;

  //! The path where to store external projects
  common::AbsolutePath ExternalInstalledPrefix;

  //! Resolve relative path to absolute from **ProjectRoot**
  common::AbsolutePath Resolve(const common::ProjectRelativePath &rp);

  //! Resolve relative path to absolute from **BuildRoot**
  common::AbsolutePath ResolveBuild(const common::ProjectRelativePath &rp);

  //! Returns configuration loaded from project root.
  const Configuration &Config() const;

 private:
  mutable boost::optional<Configuration> config_;
};

}  // namespace jk::core::filesystem
