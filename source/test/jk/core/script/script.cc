// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

#define JK_TEST

#include <catch.hpp>
#include <initializer_list>
#include <list>

#include "fmt/core.h"
#include "jk/core/executor/script.hh"
#include "jk/utils/str.hh"

namespace jk::core::executor::test {

static std::string FunctionCall(
    const std::string &name,
    std::initializer_list<std::pair<std::string, std::string>> args) {
  return fmt::format(
      "{}({})", name,
      utils::JoinString(", ", args.begin(), args.end(),
                        [](const std::pair<std::string, std::string> &pr) {
                          return fmt::format("{} = {}", pr.first, pr.second);
                        }));
}

static std::string StringList(std::initializer_list<std::string> args) {
  return fmt::format("[{}]", utils::JoinString(", ", args.begin(), args.end(),
                                               [](const std::string &s) {
                                                 return fmt::format("'{}'", s);
                                               }));
}

TEST_CASE("Script", "[core][script]") {
  auto interp = ScriptInterpreter::ThreadInstance();

  SECTION("simple cc_library only") {
    auto content =
        FunctionCall("cc_library", {{"name", "'base'"},
                                    {"srcs", StringList({"foo.cpp"})},
                                    {"deps", StringList({
                                                 "//bar1/BUILD:foo",
                                                 "//bar2:foo",
                                                 "bar3/BUILD:foo",
                                                 "bar3:foo",
                                                 "##bar4/BUILD:foo",
                                                 "##bar5:foo",
                                                 ":bar",
                                             })}});

    auto result = interp->Eval(content);

    REQUIRE(result.size() == 1);
    const auto &r = result[0];

    REQUIRE(r.FuncName == "cc_library");
    // TODO(hawtian): add test
  }

  SECTION("no name") {
    auto content =
        FunctionCall("cc_library", {{"srcs", StringList({"foo.cpp"})},
                                    {"deps", StringList({
                                                 "//bar1/BUILD:foo",
                                                 "//bar2:foo",
                                                 "bar3/BUILD:foo",
                                                 "bar3:foo",
                                                 "##bar4/BUILD:foo",
                                                 "##bar5:foo",
                                                 ":bar",
                                             })}});

    REQUIRE_THROWS(interp->Eval(content));
  }
}

}  // namespace jk::core::executor::test
