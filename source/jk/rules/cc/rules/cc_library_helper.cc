// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_library_helper.hh"

#include "jk/core/rules/package.hh"

namespace jk::rules::cc {

std::list<std::string> MergeDepHeaders(CCLibrary *rule,
                                       core::filesystem::JKProject *project) {
  std::list<std::string> all_dep_headers;

  for (auto dep : rule->DependenciesInOrder()) {
    auto vec = dep->ExportedHeaders();
    std::transform(
        vec.begin(), vec.end(), std::back_inserter(all_dep_headers),
        [project, dep](const std::string &filename) {
          return project->Resolve(dep->Package->Path.Sub(filename)).Stringify();
        });
  }

  return all_dep_headers;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
