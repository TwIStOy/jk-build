// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include "jk/common/path.hh"

namespace jk {
namespace core {
namespace filesystem {

struct ProjectFileSystem {
  //! Root path of this project. All packages path will be relative with this.
  common::AbsolutePath ProjectRoot;

  //! Root path of build environment. Default is '.build' in **ProjectRoot**.
  common::AbsolutePath BuildRoot;

  //! Resolve relative path to absolute from **ProjectRoot**
  common::AbsolutePath Resolve(const common::ProjectRelativePath &rp);

  //! Resolve relative path to absolute from **BuildRoot**
  common::AbsolutePath ResolveBuild(const common::ProjectRelativePath &rp);
};

fs::path ProjectRoot();

fs::path BuildRoot();

}  // namespace filesystem
}  // namespace core
}  // namespace jk
