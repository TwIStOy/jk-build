// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "nlohmann/json.hpp"

namespace jk::core::cache {

using nlohmann::json;

struct CacheSlot {
  std::optional<std::string> __current;
  std::unordered_map<std::string, std::unique_ptr<CacheSlot>> __next;

  inline CacheSlot *Next(const std::string &k) {
    if (auto it = __next.find(k); it != __next.end()) {
      return it->second.get();
    }
    auto ptr = new CacheSlot;
    __next[k].reset(ptr);
    return ptr;
  }
};

void to_json(json &j, const CacheSlot &p);
void from_json(const json &j, CacheSlot &p);

class JKCache {
 public:
  template<typename... Args,
           typename = std::enable_if_t<
               (std::is_convertible_v<Args &&, std::string> && ...)>>
  std::optional<std::string> &GetKey(Args &&... args) {
    return GetKey({args...});
  }

  inline void Flush(filesystem::JKProject *project) {
    json j = slot_;
    {
      std::ofstream ofs(project->BuildRoot.Sub("jk_cache.json").Stringify());
      ofs << j;
    }
  }

 private:
  std::optional<std::string> &GetKey(std::list<std::string> keys);

 private:
  CacheSlot slot_;
};

}  // namespace jk::core::cache

// vim: fdm=marker
