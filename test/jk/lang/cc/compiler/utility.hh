// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "boost/optional.hpp"
#include "jk/core/output/makefile.hh"

namespace jk::lang::cc::test {

inline boost::optional<core::output::UnixMakefile::IncludeItem> FindInclude(
    std::list<core::output::UnixMakefile::IncludeItem> includes,
    const fs::path &path) {
  for (const auto &it : includes) {
    if (it.Path == path) {
      return it;
    }
  }
  return {};
}

inline std::list<std::string> MergeDependencies(
    const std::list<core::output::UnixMakefile::TargetItem> &targets,
    const std::string &name) {
  std::list<std::string> res;
  for (const auto &target : targets) {
    if (target.TargetName == name) {
      res.insert(res.end(), target.Dependencies.begin(),
                 target.Dependencies.end());
    }
  }
  return res;
}

}  // namespace jk::lang::cc::test

// vim: fdm=marker

