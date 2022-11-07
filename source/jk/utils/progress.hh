// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::utils {

class ProgressBar {
 public:
  inline explicit ProgressBar(uint32_t bar_width, char base = '=',
                              bool arrow = true)
      : base_(base), arrow_(arrow) {
  }

  inline void Print(std::ostream &out, uint64_t now, uint64_t total,
                    const std::string &msg) {
    auto percent = static_cast<double>(now) / total;
    if (total == 0) {
      percent = 0;
    }
    auto width = bar_width_;
    width -= msg.length() + 1;

    int32_t place_numbers = width * percent;
    place_numbers         = std::min<int32_t>(place_numbers, width);

    std::ostringstream oss;

    if (!first_print_) {
      oss << "\r";
    }

    first_print_ = false;
    if (arrow_) {
      place_numbers--;

      place_numbers = std::max<int32_t>(place_numbers, 0);
    }
    auto rest = width;

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
    oss << " " << msg;

    out << oss.str();
    out.flush();
  }

 private:
  uint32_t bar_width_;
  char base_;
  bool arrow_;
  bool first_print_{true};
};

}  // namespace jk::utils

// vim: fdm=marker
