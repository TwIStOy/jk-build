// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/mm_parser.hh"

#include <iostream>

#include "catch.hpp"

namespace jk::core::gnu::test {

TEST_CASE("mm parser", "[utils][parser][mm]") {
  SECTION("case 1") {
    auto case_str = R"(main.o: abc.h bcd.h ddd.h)";

    auto res = MM::Parse(case_str);

    std::cout << "Result: " << res->Stringify() << std::endl;

    REQUIRE(res);
    REQUIRE(res->Target == "main.o");
    REQUIRE(res->Dependencies.size() == 3);
    REQUIRE(res->Dependencies[0] == "abc.h");
    REQUIRE(res->Dependencies[1] == "bcd.h");
    REQUIRE(res->Dependencies[2] == "ddd.h");
  }

  SECTION("case 2") {
    auto case_str = R"(main.o: abc.h \
                            bcd.h \
                            ddd.h
    )";

    auto res = MM::Parse(case_str);

    std::cout << "Result: " << res->Stringify() << std::endl;

    REQUIRE(res);
    REQUIRE(res->Target == "main.o");
    REQUIRE(res->Dependencies.size() == 3);
    REQUIRE(res->Dependencies[0] == "abc.h");
    REQUIRE(res->Dependencies[1] == "bcd.h");
    REQUIRE(res->Dependencies[2] == "ddd.h");
  }

  SECTION("case 3") {
    auto case_str = R"(m\ ain.o: abc.h \
                            bcd.h \
                            ddd.h \
                            abc/cde.h \
                            abc\ bbb.h
    )";

    auto res = MM::Parse(case_str);

    std::cout << "Result: " << res->Stringify() << std::endl;

    REQUIRE(res);
    REQUIRE(res->Target == "m ain.o");
    REQUIRE(res->Dependencies.size() == 5);
    REQUIRE(res->Dependencies[0] == "abc.h");
    REQUIRE(res->Dependencies[1] == "bcd.h");
    REQUIRE(res->Dependencies[2] == "ddd.h");
    REQUIRE(res->Dependencies[3] == "abc/cde.h");
    REQUIRE(res->Dependencies[4] == "abc bbb.h");
  }
}

}  // namespace jk::core::gnu::test

// vim: fdm=marker
