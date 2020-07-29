// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/cache/cache.hh"

#include <fstream>
#include <string>

#include "boost/functional/hash.hpp"
#include "jk/core/filesystem/project.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/utils/array.hh"

namespace jk::core::cache {

void to_json(json &j, const BuildRuleCache &r) {
  j = json{{"name", r.Name}, {"source_files", r.SourceFiles}};
}

void from_json(const json &j, BuildRuleCache &r) {
  j.at("name").get_to(r.Name);
  j.at("source_files").get_to(r.SourceFiles);
}

CacheDatabase::CacheDatabase() {
}

bool CacheDatabase::IsUpToDate(rules::BuildRule *rule,
                               filesystem::ProjectFileSystem *project,
                               filesystem::FileNamePatternExpander *expander) {
  if (rule->Type.IsCC()) {
    return true;
  }
  // load cache first
  auto it = rule_cache_.find(rule->FullQualifiedName());
  if (it == rule_cache_.end()) {
    // if not exists, return false
    return false;
  }

  auto cc = rule->Downcast<rules::CCLibrary>();
  auto source_files = cc->ExpandSourceFiles(project, expander);

  return utils::SameArray(it->second.SourceFiles, source_files);
}

// std::size_t LoadFile(common::AbsolutePath file) {
//   std::size_t res;
//   std::ifstream ifs(file.Stringify());
//   std::string line;
//   while (std::getline(ifs, line)) {
//     boost::hash_combine(res, line);
//   }
//   return res;
// }
//
// bool CacheDatabase::IsUpToDate(common::AbsolutePath file) {
//   auto hash_value = LoadFile(file);
//   auto it = file_hash_.find(file.Stringify());
//   if (it == file_hash_.end()) {
//   }
// }

}  // namespace jk::core::cache

// vim: fdm=marker

