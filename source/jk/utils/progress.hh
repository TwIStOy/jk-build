// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <unistd.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "jk/common/flags.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::utils {

class ProgressBar {
 public:
  inline explicit ProgressBar(uint32_t total, uint32_t bar_width = 0,
                              char base = '=', bool arrow = true)
      : total_(total), base_(base), arrow_(arrow) {
    if (bar_width == 0) {
      bar_width_ = common::FLAGS_terminal_columns - 7;
    }
  }

  inline void Update(uint32_t now, uint32_t total) {
    now_ = now;
    total_ = total;
  }

  inline void Print(std::ostream &out) {
    auto percent = static_cast<double>(now_) / total_;
    int32_t place_numbers = bar_width_ * percent;

    std::ostringstream oss;

    if (!first_print_) {
      oss << "\r";
    }

    first_print_ = false;
    if (arrow_) {
      place_numbers--;

      place_numbers = std::max<int32_t>(place_numbers, 0);
    }
    auto rest = bar_width_;

    oss << '[';
    for (auto i = 0; i < place_numbers; i++) {
      oss << base_;
      rest--;
    }
    if (arrow_ && place_numbers > 0) {
      oss << '>';
      rest--;
    }

    for (auto i = 0u; i < rest; i++) {
      oss << ' ';
    }
    oss << fmt::format("] {:3d}%", static_cast<int32_t>(percent * 100));

    out << oss.str();
  }

 private:
  uint32_t total_;
  uint32_t now_{0};
  uint32_t bar_width_;
  char base_;
  bool arrow_;
  bool first_print_{true};
};

}  // namespace jk::utils

// vim: fdm=marker

