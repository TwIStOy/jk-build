// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cassert>
#include <filesystem>
#include <string>
#include <utility>

#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace fs = std::filesystem;

namespace jk::common {

struct ProjectRelativePath final : public utils::Stringifiable {
  fs::path Path;

  explicit ProjectRelativePath(fs::path p) : Path(std::move(p)) {
  }

  inline fs::path ToAbsolute(const fs::path &r) {
    assert(r.is_absolute());

    return r / Path;
  }

  template<typename... Ts>
  inline ProjectRelativePath Sub(const Ts &...rhs) const {
    return ProjectRelativePath{(Path / ... / rhs)};
  }

  // inherited from |utils::Stringifiable|
  std::string gen_stringify_cache() const;
};

struct AbsolutePath final : public utils::Stringifiable {
  static AbsolutePath CurrentWorkingDirectory();

  fs::path Path;

  explicit AbsolutePath(fs::path p) : Path(std::move(p)) {
  }

  template<typename... Ts>
  inline AbsolutePath Sub(const Ts &...rhs) const {
    return AbsolutePath{(Path / ... / rhs)};
  }

  // inherited from |utils::Stringifiable|
  std::string gen_stringify_cache() const;
};

struct AbsolutePathHasher {
  inline std::size_t operator()(const AbsolutePath &p) const {
    return fs::hash_value(p.Path);
  }
};

void AssumeFolder(const fs::path &rp);

inline void AssumeFolder(const ProjectRelativePath &rp) {
  AssumeFolder(rp.Path);
}

uint32_t GetNumberOfFilesInDirectory(const AbsolutePath &path);

void RemoveDirectory(const AbsolutePath &p);

void FastWriteFile(const fs::path &rp, const std::string &content);

}  // namespace jk::common
