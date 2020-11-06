// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/parser/parser_fragment.hh"

#include <memory>

#include "catch.hpp"
#include "jk/core/parser/parser_context.hh"

namespace jk::core::parser {

template<>
struct TypePlus<int, char> {
  using type = int;
  static int Combine(int x, char y) {
    return x * 10 + y - '0';
  }
};

}  // namespace jk::core::parser

namespace jk::core::parser::test {

ParserFragment<char> Term(char ch) {
  ParserFragment<char> res;

  res.Executor =
      [ch](const ParserContext &ctx) -> ParserFragment<char>::context_t {
    ParserContext res{ctx};
    if (ctx.Peek() == ch) {
      res.CurrentIndex++;
      return {res, ch};
    }
    return {res, {}};
  };

  return res;
}

TEST_CASE("parser_fragment", "[utils][parser]") {
  auto Digit = Term('0') | Term('1') | Term('2') | Term('3') | Term('4') |
               Term('5') | Term('6') | Term('7') | Term('8') | Term('9');

  ParserFragment<int> number;
  number = (number + Digit) | Digit;

  ParserContext ctx{std::make_shared<std::string>("123"), 0};


  // ParserFragment<int> Number;
  // Number.Executor =
  //     [](const ParserContext &ctx) -> ParserFragment<int>::context_t {
  // };
}

}  // namespace jk::core::parser::test

// vim: fdm=marker

