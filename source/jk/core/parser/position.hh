// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>

namespace jk::core::parser {

struct Position {
  uint32_t Line{0};
  uint32_t Column{0};

  inline void NextLine() {
    Line++;
    Column = 0;
  }

  inline void NextColumn() {
    Column = 0;
  }
};

}  // namespace jk::core::parser

// vim: fdm=marker

