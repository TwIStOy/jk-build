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

void to_json(json &j, const CacheSlot &p) {
  j = json::object();
  if (p.__current) {
    j["__current"] = p.__current.value();
  }
  if (p.__next.size()) {
    json v;
    for (const auto &[name, subslot] : p.__next) {
      if (subslot) {
        json vv;
        to_json(vv, *subslot);
        v[name] = std::move(vv);
      }
    }
    j["__next"] = std::move(v);
  }
}

void from_json(const json &j, CacheSlot &p) {
  if (j.contains("__current")) {
    std::string curr;
    j.at("__current").get_to(curr);
    p.__current = curr;
  }

  if (j.contains("__next")) {
    const auto &__next = j.at("__next");
    for (auto it = __next.begin(); it != __next.end(); ++it) {
      std::unique_ptr<CacheSlot> subslot{new CacheSlot};
      from_json(it.value(), *subslot);
      p.__next[it.key()] = std::move(subslot);
    }
  }
}

std::optional<std::string> &JKCache::GetKey(std::list<std::string> keys) {
  auto now = &slot_;
  while (keys.size()) {
    auto k = keys.front();
    keys.erase(keys.begin());
    now = slot_.Next(k);
  }
  return slot_.__current;
}

}  // namespace jk::core::cache

// vim: fdm=marker
