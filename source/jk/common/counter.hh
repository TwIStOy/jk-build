// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <vector>

namespace jk::core::rules {

struct BuildRule;

}  // namespace jk::core::rules

namespace jk::common {

//! Global Counter
struct _Counter {
  inline uint32_t Now() {
    return Count;
  }

  uint32_t Count{0};

 private:
  friend struct jk::core::rules::BuildRule;

  inline uint32_t Next() {
    uint32_t ret = Count++;
    return ret;
  }
};

inline _Counter *Counter() {
  static _Counter cnt;
  return &cnt;
}

}  // namespace jk::common

// vim: fdm=marker

