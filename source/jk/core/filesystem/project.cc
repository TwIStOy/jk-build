// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/project.hh"

#include <filesystem>
#include <memory>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "spdlog/spdlog.h"

namespace jk::core::filesystem {

static constexpr auto ROOT_MARKER           = "JK_ROOT";
static constexpr auto OLD_STYLE_ROOT_MARKER = "BLADE_ROOT";

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

static bool HasRootMarker(const fs::path &root, const char *const marker_file) {
  auto marker = root / marker_file;
  if (std::filesystem::exists(marker.string()) &&
      std::filesystem::is_regular_file(marker.string())) {
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

const Configuration &JKProject::Config() const {
  if (config_) {
    return config_.value();
  }

  auto file = ProjectRoot.Path / ROOT_MARKER;
  if (std::filesystem::exists(file.string())) {
    config_ = Configuration(this, toml::parse(file.string()));
  } else {
    config_ = Configuration(this, toml::value());
  }

  return config_.value();
}

std::unique_ptr<JKProject> JKProject::ResolveFrom(
    const common::AbsolutePath &cwd) {
  auto current = cwd.Path;
  while (current.parent_path() != current) {
    utils::Logger("jk")->debug(R"(Checking folder "{}"...)", current.string());
    if (HasRootMarker(current, ROOT_MARKER)) {
      utils::Logger("jk")->info("Project Root: {}", current.string());
      return std::make_unique<JKProject>(common::AbsolutePath{current});
    } else if (HasRootMarker(current, OLD_STYLE_ROOT_MARKER)) {
      // backward compatibility
      utils::Logger("jk")->info("Project Root(Old Style): {}",
                                current.string());
      auto project = std::make_unique<JKProject>(common::AbsolutePath{current});
      project->old_style_ = true;
      return project;
    }
    current = current.parent_path();
  }

  JK_THROW(JKBuildError("You are not in a JK project. No JK_ROOT file found."));
}

}  // namespace jk::core::filesystem

// vim: fdm=marker
