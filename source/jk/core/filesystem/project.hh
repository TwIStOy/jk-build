// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <optional>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/filesystem/configuration.hh"
#include "toml.hpp"

namespace jk::core::filesystem {

enum class TargetPlatform {
  k32,
  k64,
};

inline std::string ToString(TargetPlatform plt) {
  if (plt == TargetPlatform::k32) {
    return "32";
  } else {
    return "64";
  }
}

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
  template<typename... Args>
  common::AbsolutePath Resolve(Args &&...args) {
    return (ProjectRoot.Path / ... / std::forward<Args>(args));
  }

  //! Resolve relative path to absolute from **BuildRoot**
  template<typename... Args>
  common::AbsolutePath ResolveBuild(Args &&...args) {
    return (BuildRoot.Path / ... / std::forward<Args>(args));
  }

  //! Returns configuration loaded from project root.
  const Configuration &Config() const;

  // -- flags section --
  bool AlwaysCheckPattern() const;

  //! Returns if its old-style project.
  bool IsOldStyle() const;

 private:
  bool old_style_{false};
  mutable std::optional<Configuration> config_;
};

inline bool JKProject::IsOldStyle() const {
  return old_style_;
}

}  // namespace jk::core::filesystem
