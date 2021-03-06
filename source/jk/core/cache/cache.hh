// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <unordered_map>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "nlohmann/json.hpp"

namespace jk::core::cache {

using nlohmann::json;

struct BuildRuleCache {
  std::string Name;
  std::vector<std::string> SourceFiles;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BuildRuleCache, Name, SourceFiles);

class CacheDatabase {
 public:
  explicit CacheDatabase(const common::AbsolutePath &file_path);

  bool IsUpToDate(rules::BuildRule *rule, filesystem::JKProject *project,
                  filesystem::FileNamePatternExpander *expander);

  void WriteCache(common::AbsolutePath file_path);

 private:
  void LoadCacheFile(const common::AbsolutePath &file);

 private:
  std::unordered_map<std::string, BuildRuleCache> rule_cache_;
  std::unordered_map<std::string, std::vector<std::string>> package_cache_;
};

}  // namespace jk::core::cache

// vim: fdm=marker
