// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/mm_parser.hh"

#include <iterator>
#include <list>
#include <optional>
#include <string>

#include "jk/utils/str.hh"

namespace jk::core::gnu {

std::optional<MM> MM::Parse(const std::string &text) {
  uint32_t current_index = 0;
  auto read_name = [&]() {
    std::string cache;
  };
}

}  // namespace jk::core::gnu

// vim: fdm=marker

