// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/utils/cpp_features.hh"

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

  std::vector<uint32_t> Steps() const;

  uint32_t Count() const;

 private:
  std::unordered_map<std::string, uint32_t> values_;
};

__JK_ALWAYS_INLINE uint32_t CountableSteps::Step(const std::string &name) {
  auto it = values_.find(name);
  if (it == values_.end()) {
    auto v = Counter()->Next();
    values_[name] = v;
    return v;
  }
  return it->second;
}

__JK_ALWAYS_INLINE std::vector<uint32_t> CountableSteps::Steps() const {
  std::vector<uint32_t> res;
  for (const auto &[k, v] : values_) {
    res.push_back(v);
  }
  return res;
}

__JK_ALWAYS_INLINE uint32_t CountableSteps::Count() const {
  return values_.size();
}

}  // namespace jk::common

// vim: fdm=marker
