// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <type_traits>
#include <utility>

#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parser_result.hh"

namespace jk::core::parser {

template<typename P1, typename F,
         typename R = std::result_of_t<F(typename std::decay_t<P1>::result_t)> >
auto Convert(P1 &&p, F &&_convert) -> Parser<R> {
  return Parser<R>::Make(
      [parser = std::forward<P1>(p), convert = std::forward<F>(_convert)](
          InputStream input) -> ParseResult<R> {
        auto res = parser(input);

        if (res.Success()) {
          return ParseResult<R>(res.GetInputStream(), convert(res.Result()));
        }

        return ParseResult<R>(res.GetInputStream());
      });
}

template<typename P, typename F>
auto operator>>(P &&p, F &&f) {
  return Convert(std::forward<P>(p), std::forward<F>(f));
}

}  // namespace jk::core::parser

// vim: fdm=marker

