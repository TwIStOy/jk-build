// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <vector>

namespace jk::common {

//! Global Counter
struct _Counter {
  std::vector<uint32_t> Record;

  inline uint32_t Now() {
    return Count;
  }

  inline uint32_t Next() {
    auto ret = Count++;
    Record.push_back(ret);
    return ret;
  }

  inline void Reset() {
    Record.clear();
  }

  uint32_t Count;
};

inline _Counter *Counter() {
  static _Counter cnt;
  return &cnt;
}

}  // namespace jk::common

// vim: fdm=marker

