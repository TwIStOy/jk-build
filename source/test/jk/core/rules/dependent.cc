// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/models/dependent.hh"

#include <boost/optional/optional_io.hpp>
#include <catch.hpp>

namespace jk::core::models::test {

#define DEP_PARSE(dep, pkg, rule, pos)                  \
  do {                                                  \
    auto res = ParseIdString(dep);                      \
                                                        \
    REQUIRE(res.PackageName);                           \
    REQUIRE(*res.PackageName == pkg);                   \
    REQUIRE(res.RuleName == rule);                      \
    REQUIRE(res.Position == RuleRelativePosition::pos); \
  } while (0);

TEST_CASE("Parse Absolute Id", "[core][rules][dependent]") {
  DEP_PARSE("//media_server_library/base:base", "media_server_library/base",
            "base", kAbsolute);
  DEP_PARSE(
      "//agora_universal_transport/third_party/tiny-AES-c/BUILD:tiny-AES-c",
      "agora_universal_transport/third_party/tiny-AES-c", "tiny-AES-c",
      kAbsolute);
}

TEST_CASE("Parse Relative Id", "[core][rules][dependent]") {
  DEP_PARSE("media_server_library/base:base", "media_server_library/base",
            "base", kRelative);
}

TEST_CASE("Parse This Id", "[core][rules][dependent]") {
  auto res = ParseIdString(":base");

  REQUIRE_FALSE(res.PackageName);
  REQUIRE(res.RuleName == "base");
  REQUIRE(res.Position == RuleRelativePosition::kThis);
}

TEST_CASE("Parse Builtin Id", "[core][rules][dependent]") {
  DEP_PARSE("##media_server_library/base:base", "media_server_library/base",
            "base", kBuiltin);
}

}  // namespace jk::core::models::test
