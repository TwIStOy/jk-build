// Copyright (c) 2020 Hawtian Wang
//

#include <memory>

#include "catch.hpp"
#include "jk/core/parser/combinator/or.hh"
#include "jk/core/parser/combinator/plus.hh"
#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parsers.hh"

namespace jk::core::parser::test {

TEST_CASE("parser_fragment", "[utils][parser]") {
  auto ab = MakeCharEq('a') + MakeCharEq('b');

  auto res = ab(InputStream{"ab"});

  REQUIRE(res.Success());

  auto Digit = MakeCharEq('0') | MakeCharEq('1') | MakeCharEq('2') |
               MakeCharEq('3') | MakeCharEq('4') | MakeCharEq('5') |
               MakeCharEq('6') | MakeCharEq('7') | MakeCharEq('8') |
               MakeCharEq('9');

  // ParserFragment<int> number;
  // number = (number + Digit) | Digit;
  //
  // ParserContext ctx{std::make_shared<std::string>("123"), 0};

  // ParserFragment<int> Number;
  // Number.Executor =
  //     [](const ParserContext &ctx) -> ParserFragment<int>::context_t {
  // };
}

}  // namespace jk::core::parser::test

// vim: fdm=marker

