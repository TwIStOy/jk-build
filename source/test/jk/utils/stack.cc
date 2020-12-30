// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/stack.hh"

#include <iostream>
#include <list>

#include "catch.hpp"

namespace jk::utils::test {

TEST_CASE("Collision Stack", "[utils]") {
  CollisionNameStack cstack;

  SECTION("Simple Push") {
    REQUIRE(cstack.Push("name1"));
    REQUIRE(cstack.Push("name2"));
    REQUIRE(cstack.Push("name3"));
    REQUIRE(cstack.Push("name4"));

    INFO(cstack.DumpStack());
  }

  SECTION("Push with collision") {
    std::list<std::string> collisions;
    REQUIRE(cstack.Push("name1"));
    REQUIRE(cstack.Push("name2"));
    REQUIRE(cstack.Push("name3"));
    REQUIRE(cstack.Push("name4"));
    REQUIRE_FALSE(cstack.Push("name1", &collisions));

    REQUIRE(collisions.size() == 4);
  }

  SECTION("Safe pop") {
    REQUIRE(cstack.Push("name1"));
    REQUIRE(cstack.Push("name2"));
    REQUIRE(cstack.Push("name3"));
    REQUIRE(cstack.Push("name4"));
    cstack.Pop();
    cstack.Pop();
    REQUIRE(cstack.Push("name3"));
  }

  SECTION("Scoped Push") {
    REQUIRE(cstack.Push("name1"));
    REQUIRE(cstack.Push("name2"));
    { auto sc = cstack.ScopedPush("name3"); }
    { auto sc = cstack.ScopedPush("name4"); }
    REQUIRE(cstack.Push("name3"));
  }
}

}  // namespace jk::utils::test
