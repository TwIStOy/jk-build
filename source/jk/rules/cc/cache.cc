// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/cache.hh"

#include <filesystem>
#include <fstream>

namespace jk::rules::cc::cache {

PackageInfo PackageCacheFromFile(const common::AbsolutePath &p) {
  std::ifstream ifs(p.Path);
  nlohmann::json j;
  ifs >> j;
  return j;
}

}  // namespace jk::rules::cc::cache

// vim: fdm=marker
