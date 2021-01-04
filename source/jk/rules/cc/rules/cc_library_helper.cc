// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_library_helper.hh"

#include <algorithm>
#include <list>
#include <string>
#include <unordered_set>

#include "jk/core/rules/package.hh"

namespace jk::rules::cc {

std::list<std::string> MergeDepHeaders(CCLibrary *rule,
                                       core::filesystem::JKProject *project) {
  std::list<std::string> all_dep_headers;

  for (auto dep : rule->DependenciesInOrder()) {
    if (dep != rule) {
      auto vec = dep->ExportedHeaders();
      std::transform(vec.begin(), vec.end(),
                     std::back_inserter(all_dep_headers),
                     [project, dep](const std::string &filename) {
                       return project->Resolve(dep->Package->Path.Sub(filename))
                           .Stringify();
                     });
    }
  }

  std::unordered_set<std::string> remove_dup{std::begin(all_dep_headers),
                                             std::end(all_dep_headers)};

  all_dep_headers =
      std::list<std::string>{std::begin(remove_dup), std::end(remove_dup)};

  return all_dep_headers;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
