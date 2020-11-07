// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <tuple>
#include <type_traits>
#include <utility>

#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parser_result.hh"
#include "jk/utils/type_traits.hh"

namespace jk::core::parser {

template<typename P1, typename P2,
         typename R = std::common_type_t<typename std::decay_t<P1>::result_t,
                                         typename std::decay_t<P2>::result_t>>
auto operator|(P1 &&p1, P2 &&p2) -> Parser<R> {
  return Parser<R>::Make(
      [lhs = std::forward<P1>(p1),
       rhs = std::forward<P2>(p2)](InputStream input) -> ParseResult<R> {
        auto lhs_res = lhs(input);
        if (lhs_res.Success()) {
          return ParseResult<R>(lhs_res.GetInputStream(),
                                std::move(lhs_res.Result()));
        }

        auto rhs_res = rhs(input);
        if (rhs_res.Success()) {
          return ParseResult<R>(rhs_res.GetInputStream(),
                                std::move(rhs_res.Result()));
        } else {
          return ParseResult<R>(rhs_res.GetInputStream());
        }
      });
}

template<typename... Parsers, typename R = std::common_type_t<
                                  typename std::decay_t<Parsers>::result_t...>>
auto Or(Parsers &&...parsers) -> Parser<R> {
  return (std::forward<Parsers>(parsers) | ...);
}

}  // namespace jk::core::parser

// vim: fdm=marker

