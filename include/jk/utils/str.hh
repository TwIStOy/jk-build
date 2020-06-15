// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <sstream>
#include <string>
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

template<typename InputIterator>
typename InputIterator::value_type __DefaultUnary(
    const typename InputIterator::value_type &v) {
  return v;
}

template<typename InputIterator,
         typename UnaryOperator = decltype(__DefaultUnary<InputIterator>)>
inline std::string JoinString(
    std::string separator, InputIterator begin, InputIterator end,
    UnaryOperator func = &__DefaultUnary<InputIterator>) {
  std::ostringstream oss;
  bool first = true;
  for (auto it = begin; it != end; ++it) {
    if (first) {
      first = false;
    } else {
      oss << separator;
    }
    oss << func(*it);
  }
  return oss.str();
}

template<typename T>
inline std::string Replace(const std::string &old, char from, const T &to) {
  std::ostringstream oss;

  for (auto x : old) {
    if (x == from) {
      oss << to;
    } else {
      oss << x;
    }
  }

  return oss.str();
}

}  // namespace utils
}  // namespace jk
