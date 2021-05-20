// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/project.hh"

#include <string>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "spdlog/spdlog.h"

namespace jk::core::filesystem {

static constexpr auto ROOT_MARKER = "JK_ROOT";

std::string ToPathSpec(TargetPlatform plt) {
  switch (plt) {
    case TargetPlatform::k32:
      return "i386";
    case TargetPlatform::k64:
      return "x86_64";
  }
  return "";
}

std::string ToExternalPathSpec(TargetPlatform plt) {
  switch (plt) {
    case TargetPlatform::k32:
      return "m32";
    case TargetPlatform::k64:
      return "m64";
  }
  return "";
}

static bool HasRootMarker(const fs::path &root) {
  auto marker = root / ROOT_MARKER;
  if (fs::exists(marker) && fs::is_regular_file(marker)) {
    return true;
  }
  return false;
}

JKProject::JKProject(common::AbsolutePath ProjectRoot, TargetPlatform Platform,
                     std::optional<common::AbsolutePath> BuildRoot)
    : ProjectRoot(ProjectRoot),
      Platform(Platform),
      BuildRoot(BuildRoot.has_value()
                    ? BuildRoot.value()
                    : ProjectRoot.Sub(".build", ToPathSpec(Platform))),
      ExternalInstalledPrefix(
          ProjectRoot.Sub(".build", ".lib", ToExternalPathSpec(Platform))) {
}

common::AbsolutePath JKProject::Resolve(const common::ProjectRelativePath &rp) {
  return ProjectRoot.Sub(rp.Path);
}

common::AbsolutePath JKProject::ResolveBuild(
    const common::ProjectRelativePath &rp) {
  return BuildRoot.Sub(rp.Path);
}

const Configuration &JKProject::Config() const {
  if (config_) {
    return config_.value();
  }

  auto file = ProjectRoot.Path / ROOT_MARKER;
  if (boost::filesystem::exists(file)) {
    config_ = Configuration(toml::parse(file.string()));
  } else {
    config_ = Configuration(toml::value());
  }

  return config_.value();
}

JKProject JKProject::ResolveFrom(const common::AbsolutePath &cwd) {
  auto current = cwd.Path;
  while (current.parent_path() != current) {
    utils::Logger("jk")->debug(R"(Checking folder "{}"...)", current.string());
    if (HasRootMarker(current)) {
      utils::Logger("jk")->info("Project Root: {}", current.string());
      return JKProject(common::AbsolutePath{current});
    }
    current = current.parent_path();
  }

  JK_THROW(JKBuildError("You are not in a JK project. No JK_ROOT file found."));
}

}  // namespace jk::core::filesystem

// vim: fdm=marker
