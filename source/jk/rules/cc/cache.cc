// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/cache.hh"

#include "boost/filesystem.hpp"

namespace jk::rules::cc::cache {

namespace fs = boost::filesystem;

PackageInfo PackageCacheFromFile(const common::AbsolutePath &p) {
  fs::ifstream ifs(p.Path);
  nlohmann::json j;
  ifs >> j;
  return j;
}

}  // namespace jk::rules::cc::cache

// vim: fdm=marker
