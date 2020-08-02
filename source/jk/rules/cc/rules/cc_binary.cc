// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_binary.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/utils/logging.hh"

namespace jk::rules::cc {
std::vector<std::string> CCBinary::ResolveDependenciesAndLdFlags(
    core::filesystem::ProjectFileSystem *project,
    const std::string &build_type) const {
  std::vector<std::string> res;

  res.insert(res.end(), std::begin(LdFlags), std::end(LdFlags));
  for (auto rule : DependenciesInOrder()) {
    auto exported_files = rule->ExportedFilesSimpleName(project, build_type);

    std::copy(std::begin(exported_files), std::end(exported_files),
              std::back_inserter(res));
    auto exported_ldflags = rule->ExportedLinkFlags();
    res.insert(res.end(), std::begin(exported_ldflags),
               std::end(exported_ldflags));
  }

  return res;
}

}  // namespace jk::rules::cc

// vim: fdm=marker

