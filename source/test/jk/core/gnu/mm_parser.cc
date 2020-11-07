// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/mm_parser.hh"

#include <iostream>

#include "catch.hpp"

namespace jk::core::gnu::test {

TEST_CASE("mm parser", "[utils][parser][mm]") {
  SECTION("case 1") {
    auto case1 = R"(main.o: abc.h bcd.h ddd.h)";

    auto res = MM::Parse(case1);

    std::cout << "Result: " << res->Stringify() << std::endl;

    REQUIRE(res);
    REQUIRE(res->Target == "main.o");
    REQUIRE(res->Dependencies.size() == 3);
    REQUIRE(res->Dependencies[0] == "abc.h");
    REQUIRE(res->Dependencies[1] == "bcd.h");
    REQUIRE(res->Dependencies[2] == "ddd.h");
  }
}

}  // namespace jk::core::gnu::test

// vim: fdm=marker

