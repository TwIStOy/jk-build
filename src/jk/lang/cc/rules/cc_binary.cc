// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/rules/cc_binary.hh"

#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/utils/logging.hh"

namespace jk::core::rules {

std::vector<std::string> CCBinary::ResolveDependenciesAndLdFlags() const {
  std::vector<std::string> res;

  res.insert(res.end(), std::begin(LdFlags), std::end(LdFlags));
  for (auto rule : DependenciesInOrder()) {
    auto exported_files = rule->ExportedFilesSimpleName();
    res.insert(res.end(), std::begin(exported_files), std::end(exported_files));
    auto exported_ldflags = rule->ExportedLinkFlags();
    res.insert(res.end(), std::begin(exported_ldflags),
               std::end(exported_ldflags));
  }

  return res;
}

}  // namespace jk::core::rules

// vim: fdm=marker

