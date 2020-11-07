// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <tuple>
#include <type_traits>
#include <utility>

#include "jk/core/parser/parser.hh"
#include "jk/utils/type_traits.hh"

namespace jk::core::parser {

template<typename P1, typename P2,
         typename R1 = std::conditional_t<
             utils::is_tuple_v<typename std::decay_t<P1>::result_t>,
             typename std::decay_t<P1>::result_t,
             std::tuple<typename std::decay_t<P1>::result_t>>,
         typename R2 = std::conditional_t<
             utils::is_tuple_v<typename std::decay_t<P2>::result_t>,
             typename std::decay_t<P2>::result_t,
             std::tuple<typename std::decay_t<P2>::result_t>>,
         typename R = decltype(std::tuple_cat(std::declval<R1>(),
                                              std::declval<R2>()))>
auto operator+(P1 &&p1, P2 &&p2) -> Parser<R> {
  return Parser<R>::Make([lhs = std::forward<P1>(p1),
                          rhs = std::forward<P2>(p2)](
                             InputStream input) -> ParseResult<R> {
    auto ret = ParseResult<R>(input);

    auto lhs_res = lhs(input);
    if (lhs_res.Success()) {
      auto rhs_res = rhs(lhs_res.GetInputStream());
      if (rhs_res.Success()) {
        ret = ParseResult<R>(rhs_res.GetInputStream(),
                             std::tuple_cat(R1(std::move(lhs_res.Result())),
                                            R2(std::move(rhs_res.Result()))));
      }
    }
    return ret;
  });
}

template<typename... Parsers, typename result_t = std::tuple<
                                  typename std::decay_t<Parsers>::result_t...>>
auto Plus(Parsers &&...parsers) -> Parser<result_t> {
  static_assert((is_parser_v<std::decay_t<Parsers>> && ...),
                "Plus only support Parsers.");
  using parser_tuple = std::tuple<std::decay_t<Parsers>...>;

  return (std::forward<Parsers>(parsers) + ...);
}

}  // namespace jk::core::parser

// vim: fdm=marker

