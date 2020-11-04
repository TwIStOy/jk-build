// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

#include <iostream>

#include "jk/utils/logging.hh"

namespace jk::common {

std::string ProjectRelativePath::Stringify() const {
  return Path.string();
}

std::string AbsolutePath::Stringify() const {
  return Path.string();
}

uint32_t GetNumberOfFilesInDirectory(const AbsolutePath &p) {
  fs::directory_iterator begin_it{p.Path};
  fs::directory_iterator end_it{};

  size_t cnt = 0;
  for (; begin_it != end_it; ++begin_it) {
    cnt++;
  }
  return cnt;
}

void RemoveDirectory(const AbsolutePath &p) {
  fs::remove_all(p.Path);
}

auto logger = utils::Logger("path");

void AssumeFolder(const fs::path &rp) {
  if (!rp.is_absolute()) {
    AssumeFolder(fs::current_path() / rp);
    return;
  }

  if (rp == "/") {
    return;
  }

  if (fs::exists(rp)) {
    if (fs::is_directory(rp)) {
      return;
    }

    JK_THROW(
        core::JKBuildError("{} is exist, but not a directory", rp.string()));
  }

  AssumeFolder(rp.parent_path());
  fs::create_directory(rp);
}

}  // namespace jk::common

// vim: fdm=marker

