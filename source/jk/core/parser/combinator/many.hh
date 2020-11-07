// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <iostream>
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

      std::cout << "Input: " << current_input.GetInput()
                << ", Succ: " << res.Success() << std::endl;

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

}  // namespace jk::core::parser

// vim: fdm=marker

