// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

#include <iostream>

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

}  // namespace jk::common

// vim: fdm=marker

