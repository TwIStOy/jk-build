// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cassert>
#include <string>
#include <utility>

#include "boost/filesystem.hpp"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace fs = boost::filesystem;

namespace jk::common {

struct ProjectRelativePath final : public utils::Stringifiable {
  fs::path Path;

  explicit ProjectRelativePath(fs::path p) : Path(std::move(p)) {
  }

  inline fs::path ToAbsolute(const fs::path &r) {
    assert(r.is_absolute());

    return r / Path;
  }

  template<typename T>
  inline ProjectRelativePath Sub(const T &rhs) const {
    return ProjectRelativePath{Path / rhs};
  }

  // inherited from |utils::Stringifiable|
  std::string Stringify() const;
};

struct AbsolutePath final : public utils::Stringifiable {
  fs::path Path;

  explicit AbsolutePath(fs::path p) : Path(std::move(p)) {
  }

  template<typename T>
  inline AbsolutePath Sub(const T &rhs) const {
    return AbsolutePath{Path / rhs};
  }

  // inherited from |utils::Stringifiable|
  std::string Stringify() const final;
};

void AssumeFolder(const fs::path &rp);

inline void AssumeFolder(const ProjectRelativePath &rp) {
  AssumeFolder(rp.Path);
}

uint32_t GetNumberOfFilesInDirectory(const AbsolutePath &path);

void RemoveDirectory(const AbsolutePath &p);

}  // namespace jk::common

