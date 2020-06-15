// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cassert>

#include "jk/core/error.h"

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace jk::common {

struct ProjectRelativePath {
  fs::path Path;

  inline fs::path ToAbsolute(const fs::path &r) {
    assert(r.is_absolute());

    return r / Path;
  }

  template<typename T>
  inline ProjectRelativePath Sub(const T &rhs) {
    return ProjectRelativePath{Path / rhs};
  }

  inline std::string str() {
    return Path.string();
  }
};

struct AbsolutePath {
  fs::path Path;

  template<typename T>
  inline AbsolutePath Sub(const T &rhs) {
    return AbsolutePath{Path / rhs};
  }

  inline std::string str() {
    return Path.string();
  }
};

inline void AssumeFolder(const fs::path &rp) {
  if (fs::exists(rp)) {
    if (fs::is_directory(rp)) {
      return;
    }
    throw core::JKBuildError("{} is exist, but not a directory", rp.string());
  }

  AssumeFolder(rp.parent_path());
  fs::create_directory(rp);
}

inline void AssumeFolder(const ProjectRelativePath &rp) {
  AssumeFolder(rp.Path);
}

}  // namespace jk::common

