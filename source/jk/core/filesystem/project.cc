// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/project.hh"

#include <string>

#include "boost/optional.hpp"
#include "jk/common/flags.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "spdlog/spdlog.h"

namespace jk::core::filesystem {

JKProject::JKProject(common::AbsolutePath ProjectRoot,
                     common::AbsolutePath BuildRoot)
    : ProjectRoot(ProjectRoot), BuildRoot(BuildRoot) {
}

common::AbsolutePath JKProject::Resolve(const common::ProjectRelativePath &rp) {
  return ProjectRoot.Sub(rp.Path);
}

common::AbsolutePath JKProject::ResolveBuild(
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

const Configuration &JKProject::Config() const {
  if (config_) {
    return config_.value();
  }

  auto file = ProjectRoot.Path / "JK_ROOT";
  if (boost::filesystem::exists(file)) {
    config_ = Configuration(toml::parse(file.string()));
  } else {
    config_ = Configuration(toml::value());
  }

  return config_.value();
}

static boost::optional<fs::path> _ProjectRoot;

fs::path ProjectRoot() {
  if (_ProjectRoot) {
    return _ProjectRoot.value();
  }

  auto current = fs::current_path();
  while (current.parent_path() != current) {
    utils::Logger("jk")->debug(R"(Checking folder "{}"...)", current.string());
    if (HasRootMarker(current)) {
      utils::Logger("jk")->info("Project Root: {}", current.string());
      _ProjectRoot = current;
      return current;
    }
    current = current.parent_path();
  }

  JK_THROW(JKBuildError("You are not in a JK project. No JK_ROOT file found."));
}

fs::path BuildRoot() {
  if (common::FLAGS_platform == common::Platform::k32) {
    return ProjectRoot() / ".build" / "i386";
  }
  return ProjectRoot() / ".build" / "x86_64";
}

common::AbsolutePath JKProject::ExternalInstalledPrefix() {
  return ProjectRoot.Sub(".build").Sub(".lib").Sub(fmt::format(
      "m{}", common::FLAGS_platform == common::Platform::k32 ? 32 : 64));
}

}  // namespace jk::core::filesystem

// vim: fdm=marker
