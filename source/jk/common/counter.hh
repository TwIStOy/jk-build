// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/utils/cpp_features.hh"
#include "range/v3/view/map.hpp"

namespace jk::common {

//! Global Counter
struct _Counter {
  __JK_ALWAYS_INLINE uint32_t Now() {
    return Count;
  }

 private:
  uint32_t Count{0};

  friend class CountableSteps;

  __JK_ALWAYS_INLINE uint32_t Next() {
    uint32_t ret = Count++;
    return ret;
  }
};

__JK_ALWAYS_INLINE _Counter *Counter() {
  static _Counter cnt;
  return &cnt;
}

class CountableSteps {
 public:
  uint32_t Step(const std::string &name);

  inline auto Steps() const {
    return values_ | ranges::views::values;
  }

  uint32_t Count() const;

 private:
  absl::flat_hash_map<std::string, uint32_t> values_;
};

__JK_ALWAYS_INLINE uint32_t CountableSteps::Step(const std::string &name) {
  auto it = values_.find(name);
  if (it == values_.end()) {
    auto v        = Counter()->Next();
    values_[name] = v;
    return v;
  }
  return it->second;
}

__JK_ALWAYS_INLINE uint32_t CountableSteps::Count() const {
  return values_.size();
}

}  // namespace jk::common

// vim: fdm=marker
