// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/str.hh"

#include <string>
#include <vector>

#include "catch.hpp"
#include "fmt/core.h"

namespace jk::utils::test {

TEST_CASE("Check ending", "[utils]") {
  REQUIRE(StringEndWith("TESTABC", "ABC"));
  REQUIRE_FALSE(StringEndWith("TESTABC", "ABCD"));
}

TEST_CASE("Join String", "[utils]") {
  std::vector<std::string> vec{"a", "b", "c", "d"};
  REQUIRE(JoinString(", ", vec.begin(), vec.end()) == "a, b, c, d");
  REQUIRE(JoinString(", ", vec.begin(), vec.end(), [](const std::string &x) {
            return fmt::format("'{}'", x);
          }) == "'a', 'b', 'c', 'd'");
}

}  // namespace jk::utils::test

