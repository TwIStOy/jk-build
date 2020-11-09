// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/py_parser.hh"

#include <iostream>
#include <string_view>

#include "catch.hpp"
#include "jk/core/parser/input_stream.hh"
#include "jk/utils/str.hh"

namespace jk::core::gnu::test {

TEST_CASE("py parser", "[utils][parser][py]") {
  SECTION("identifier") {
    auto res = Py::ParseIdentifier("cc_library");
    REQUIRE(res);
    REQUIRE(res.value() == "cc_library");
  }

  SECTION("identifier") {
    auto res = Py::ParseIdentifier("1cc_library");
    REQUIRE(!res.has_value());
  }

  SECTION("string literal") {
    auto res = Py::ParseStringLiteral(R"("abc")");
    REQUIRE(res);
    REQUIRE(res.value() == "abc");
  }

  SECTION("string literal") {
    auto res = Py::ParseStringLiteral(R"("a\nbc")");
    REQUIRE(res);
    REQUIRE(res.value() == "a\nbc");
  }

  SECTION("string literal") {
    auto res = Py::ParseStringLiteral(R"("a\n\"bc")");
    REQUIRE(res);
    REQUIRE(res.value() == "a\n\"bc");
  }

  SECTION("string literal list") {
    auto res = Py::ParseStringList(R"(["abc", "bcd", "\n123"])");
    REQUIRE(res);
    REQUIRE(res.value().size() == 3);
    REQUIRE(res.value().at(0) == "abc");
    REQUIRE(res.value().at(1) == "bcd");
    REQUIRE(res.value().at(2) == "\n123");
  }

  SECTION("string literal list") {
    auto res = Py::ParseStringList(R"(["abc", "bcd", "\n123", ])");
    REQUIRE(res);
    REQUIRE(res.value().size() == 3);
    REQUIRE(res.value().at(0) == "abc");
    REQUIRE(res.value().at(1) == "bcd");
    REQUIRE(res.value().at(2) == "\n123");
  }

  SECTION("argument") {
    auto res = Py::ParseArgument(R"(abc = ["abc", "bcd", "\n123", ])");
    REQUIRE(res);
    REQUIRE(res.value().first == "abc");
    REQUIRE(res.value().second.index() == 1);
    REQUIRE(std::get<1>(res.value().second).size() == 3);
    REQUIRE(std::get<1>(res.value().second).at(0) == "abc");
    REQUIRE(std::get<1>(res.value().second).at(1) == "bcd");
    REQUIRE(std::get<1>(res.value().second).at(2) == "\n123");
  }

  SECTION("argument") {
    auto res = Py::ParseArgument(R"(abc = "Abc")");
    REQUIRE(res);
    REQUIRE(res.value().first == "abc");
    REQUIRE(res.value().second.index() == 0);
    REQUIRE(std::get<0>(res.value().second) == "Abc");
  }

  SECTION("func call") {
    auto res = Py::Parse(R"(cc_library(abc = "Abc"))");
    REQUIRE(res);
    REQUIRE(res.value().Functions.size() == 1);
    REQUIRE(res.value().Functions[0].Name == "cc_library");
    REQUIRE(res.value().Functions[0].Kwargs.size() == 1);

    auto it = res.value().Functions[0].Kwargs.find("abc");
    REQUIRE(it != res.value().Functions[0].Kwargs.end());
    REQUIRE(it->second.index() == 0);
    REQUIRE(std::get<0>(it->second) == "Abc");
  }
}

}  // namespace jk::core::gnu::test

// vim: fdm=marker

