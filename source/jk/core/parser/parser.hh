// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser_result.hh"

namespace jk::core::parser {

template<typename T>
struct Parser {
  using result_t = T;
  using parse_result_t = ParseResult<T>;

  using func_t = std::function<parse_result_t(InputStream)>;

  template<typename U>
  static Parser<T> Make(U &&func);

  parse_result_t operator()(InputStream input) const;

 private:
  func_t _action;
};

template<typename T>
template<typename U>
Parser<T> Parser<T>::Make(U &&func) {
  Parser<T> res;
  res._action = std::forward<U>(func);
  return res;
}

template<typename T>
inline typename Parser<T>::parse_result_t Parser<T>::operator()(
    InputStream input) const {
  return _action(input);
}

template<typename T>
struct is_parser : std::false_type {};

template<typename T>
struct is_parser<Parser<T>> : std::true_type {};

template<typename T>
constexpr static bool is_parser_v = is_parser<T>::value;

}  // namespace jk::core::parser

// vim: fdm=marker

