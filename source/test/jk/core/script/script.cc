// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

#define JK_TEST

#include <catch.hpp>
#include <initializer_list>
#include <list>

#include "fmt/core.h"
#include "jk/core/script/script.hh"
#include "jk/utils/str.hh"

namespace jk::core::script::test {

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
  auto interp = ScriptInterpreter::Instance();

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

    rules::BuildPackage pkg("test", common::ProjectRelativePath{""});

    filesystem::JKProject dummy(common::AbsolutePath(""));
    interp->EvalScriptContent(&dummy, &pkg, content);

    REQUIRE(pkg.Rules.size() == 1);
    auto it = pkg.Rules.find("base");
    REQUIRE(it != pkg.Rules.end());
    auto rule = it->second.get();

    REQUIRE(rule->Type.HasType(rules::RuleTypeEnum::kLibrary));
    REQUIRE(rule->TypeName == "cc_library");
    REQUIRE(rule->Name == "base");
    REQUIRE(rule->Package == &pkg);
    REQUIRE(rule->dependencies_str_.size() == 7);
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

    rules::BuildPackage pkg("test", common::ProjectRelativePath{""});

    filesystem::JKProject dummy(common::AbsolutePath(""));
    REQUIRE_THROWS(interp->EvalScriptContent(&dummy, &pkg, content));
  }
}

}  // namespace jk::core::script::test
