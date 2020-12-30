// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <iostream>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parser_result.hh"

namespace jk::core::parser {

template<typename P1,
         typename R = std::vector<typename std::decay_t<P1>::result_t>>
auto Many(P1 &&p, uint32_t min_occr = 0) -> Parser<R> {
  return Parser<R>::Make([parser = std::forward<P1>(p),
                          min_occr](InputStream input) -> ParseResult<R> {
    R ret;
    auto current_input = input;

    while (true) {
      auto res = parser(current_input);

      if (!res.Success()) {
        break;
      }

      ret.emplace_back(std::move(res).Result());
      current_input = std::move(res).GetInputStream();
    }

    if (ret.size() >= min_occr) {
      return ParseResult<R>(current_input, ret);
    }
    return ParseResult<R>(current_input);
  });
}

template<typename P1,
         typename R = std::optional<typename std::decay_t<P1>::result_t>>
auto Optional(P1 &&p) -> Parser<R> {
  return Parser<R>::Make(
      [parser = std::forward<P1>(p)](InputStream input) -> ParseResult<R> {
        auto res = parser(input);

        if (!res.Success()) {
          return ParseResult<R>(input, R{});
        }

        return ParseResult<R>(res.GetInputStream(), res.Result());
      });
}

template<typename P1,
         typename R = std::vector<typename std::decay_t<P1>::result_t>>
auto operator*(P1 &&p, uint32_t min_occr) -> Parser<R> {
  return Many(std::forward<P1>(p), min_occr);
}

template<typename P1,
         typename R = std::optional<typename std::decay_t<P1>::result_t>>
auto operator!(P1 &&p) -> Parser<R> {
  return Optional(std::forward<P1>(p));
}

}  // namespace jk::core::parser

// vim: fdm=marker
