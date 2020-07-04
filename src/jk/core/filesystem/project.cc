// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/project.hh"

#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::core::filesystem {

common::AbsolutePath ProjectFileSystem::Resolve(
    const common::ProjectRelativePath &rp) {
  return ProjectRoot.Sub(rp.Path);
}

common::AbsolutePath ProjectFileSystem::ResolveBuild(
    const common::ProjectRelativePath &rp) {
  return BuildRoot.Sub(rp.Path);
}

static bool HasRootMarker(const fs::path &root) {
  auto marker = root / "JK_ROOT";
  if (fs::exists(marker) && fs::is_regular_file(marker)) {
    return true;
  }
  return false;
}

fs::path ProjectRoot() {
  auto current = fs::current_path();

  while (current.parent_path() != current) {
    utils::Logger("jk")->info(R"(Checking folder "{}"...)", current.string());
    if (HasRootMarker(current)) {
      return current;
    }
    current = current.parent_path();
  }

  throw JKBuildError("You are not in a JK project. No JK_ROOT file found.");
}

fs::path BuildRoot() {
  return ProjectRoot() / ".build";
}

}  // namespace jk::core::filesystem

// vim: fdm=marker

