// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <unordered_map>
#include <vector>

#include "jk/common/path.hh"
#include "nlohmann/json.hpp"

namespace jk::rules::cc::cache {

using nlohmann::json;

//! Source file info in cache
struct SourceFileInfo {
  std::string FileName;               //! relative path from package
  std::vector<std::string> Includes;  //! source file include files
  std::string ContentHash;  //! md5 hash of source file content, to check if
                            //! file is updated
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SourceFileInfo, FileName, Includes,
                                   ContentHash);

//! Build rule info in cache
struct BuildRuleInfo {
  std::string Name;                         //! rule name
  std::vector<SourceFileInfo> SourceFiles;  //! resolved source files
  std::string Hash;  //! Hash value includes [srcs, includes, defines, cppflags,
                     //! cflags, cxxflags]
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuildRuleInfo, Name, SourceFiles);

struct PackageInfo {
  std::string Name;                  //! package name
  std::vector<BuildRuleInfo> Rules;  //! rules
  std::string Hash;                  //! hash value of all content
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PackageInfo, Name, Rules, Hash);

PackageInfo PackageCacheFromFile(const common::AbsolutePath &p);

}  // namespace jk::rules::cc::cache

// vim: fdm=marker
