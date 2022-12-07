// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

#include "jk/utils/logging.hh"

namespace jk::common {

std::string ProjectRelativePath::gen_stringify_cache() const {
  return Path.string();
}

std::string AbsolutePath::gen_stringify_cache() const {
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

void FastWriteFile(const fs::path &rp, const std::string &content) {
  if (fs::exists(rp)) {
    std::ifstream ifs(rp.string());
    std::string old_content;
    std::copy(std::istreambuf_iterator<char>{ifs},
              std::istreambuf_iterator<char>{},
              std::back_inserter(old_content));
    if (old_content == content) {
      return;
    }
  }
  std::ofstream ofs;
  ofs << content;
}

AbsolutePath AbsolutePath::CurrentWorkingDirectory() {
  return AbsolutePath{fs::current_path()};
}

}  // namespace jk::common

// vim: fdm=marker
