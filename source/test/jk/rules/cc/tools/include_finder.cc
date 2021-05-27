// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/tools/include_finder.hh"

#include <string_view>

#include "catch.hpp"

namespace jk::rules::cc::tools::test {

TEST_CASE("IncludeFinder", "[rules][cc][tools][include_finder]") {
  SECTION("system include") {
    std::string_view line = "  #include <test/test.h>";
    auto res = IncludeFinder::ParseIncludeLine(line);

    REQUIRE(res.has_value());
    REQUIRE(res.value() == "test/test.h");
  }

  SECTION("system include with comment") {
    std::string_view line = "  #include <test/test.h> // TEST";
    auto res = IncludeFinder::ParseIncludeLine(line);

    REQUIRE(res.has_value());
    REQUIRE(res.value() == "test/test.h");
  }

  SECTION("include with comment") {
    std::string_view line = R"(  #include "test/test.h" // TEST)";
    auto res = IncludeFinder::ParseIncludeLine(line);

    REQUIRE(res.has_value());
    REQUIRE(res.value() == "test/test.h");
  }

  SECTION("include with comment") {
    std::string_view line = R"(  #include "test/test.h" // TEST)";
    auto res = IncludeFinder::ParseIncludeLine(line);

    REQUIRE(res.has_value());
    REQUIRE(res.value() == "test/test.h");
  }
}

}  // namespace jk::rules::cc::tools::test

// vim: fdm=marker
