// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string_view>

namespace jk {
namespace utils {

inline bool StringEndWith(std::string_view full_string,
                          std::string_view ending) {
  if (full_string.length() >= ending.length()) {
    return (0 == full_string.compare(full_string.length() - ending.length(),
                                     ending.length(), ending));
  } else {
    return false;
  }
}

}  // namespace utils
}  // namespace jk
