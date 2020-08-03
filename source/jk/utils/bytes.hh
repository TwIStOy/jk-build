// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>

#include "fmt/format.h"

namespace jk::utils {

inline std::string BytesCount(uint64_t x) {
  if (x < 1024) {
    return fmt::format("{}", x);
  } else if (x < 1024 * 1024) {
    return fmt::format("{:.2f}Kb", x / 1024.0);
  } else if (x < 1024 * 1024 * 1024) {
    return fmt::format("{:.2f}Mb", x / 1024.0 / 1024.0);
  }
  return fmt::format("{:.2f}Gb", x / 1024.0 / 1024.0 / 1024.0);
}

}  // namespace jk::utils

// vim: fdm=marker

