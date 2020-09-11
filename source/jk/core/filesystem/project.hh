// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>

#include "jk/common/path.hh"
#include "toml.hpp"

namespace jk {
namespace core {
namespace filesystem {

struct ProjectFileSystem {
  ProjectFileSystem(common::AbsolutePath ProjectRoot,
                    common::AbsolutePath BuildRoot);

  //! Root path of this project. All packages path will be relative with this.
  common::AbsolutePath ProjectRoot;

  //! Root path of build environment. Default is '.build' in **ProjectRoot**.
  common::AbsolutePath BuildRoot;

  //! Resolve relative path to absolute from **ProjectRoot**
  common::AbsolutePath Resolve(const common::ProjectRelativePath &rp);

  //! Resolve relative path to absolute from **BuildRoot**
  common::AbsolutePath ResolveBuild(const common::ProjectRelativePath &rp);

  common::AbsolutePath ExternalInstalledPrefix();

  const toml::value &Configuration() const;

 private:
  mutable boost::optional<toml::value> config_;
};

fs::path ProjectRoot();

fs::path BuildRoot();

}  // namespace filesystem
}  // namespace core
}  // namespace jk

