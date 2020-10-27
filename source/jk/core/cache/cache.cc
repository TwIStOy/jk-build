// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/cache/cache.hh"

#include <fstream>
#include <string>

#include "boost/functional/hash.hpp"
#include "jk/common/path.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/utils/array.hh"
#include "jk/utils/logging.hh"

namespace jk::core::cache {

static auto logger = utils::Logger("cache");

CacheDatabase::CacheDatabase(const common::AbsolutePath &file_path) {
  LoadCacheFile(file_path);
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

  auto cc = rule->Downcast<::jk::rules::cc::CCLibrary>();
  auto source_files = cc->ExpandSourceFiles(project, expander);

  return utils::SameArray(it->second.SourceFiles, source_files);
}

void CacheDatabase::LoadCacheFile(const common::AbsolutePath &file) {
  std::ifstream ifs(file.Stringify());
  json doc;
  try {
    if (!(ifs >> doc)) {
      logger->warn("Failed to load cache file. Old cache will be ignored.");
      return;
    }

    for (const auto &c : doc["rules"]) {
      rule_cache_[c["Name"]] = c;
    }
  } catch (...) {
    logger->warn("Failed to load cache file. Old cache will be ignored.");
  }
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
