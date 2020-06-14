// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cassert>

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
};

struct AbsolutePath {
  fs::path Path;
};

}  // namespace jk::common

