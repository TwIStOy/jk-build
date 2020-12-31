// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <catch.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/str.hh"

namespace jk::core::rules::test {

struct DummyRule final : public BuildRule {
  DummyRule(BuildPackage *pkg, std::string name)
      : BuildRule(pkg, name, {RuleTypeEnum::kBinary}, "dummy") {
  }

  bool IsStable() const final {
    return false;
  }

  std::vector<std::string> ExportedFilesSimpleName(
      filesystem::JKProject *, const std::string &) const final {
    return {};
  }

  std::vector<std::string> ExportedLinkFlags() const final {
    return {};
  }

  std::vector<std::string> ExportedHeaders() const final {
    return {};
  }
};

TEST_CASE("Calulate Deps in order with circular dependencies",
          "[core][rules][dependent]") {
  BuildPackage pkg("tmp", common::ProjectRelativePath("tmp"));

  // s -> a
  // a -> b, c, d
  // b -> c
  // d -> e
  // e -> a
  auto s = new DummyRule(&pkg, "s");
  auto a = new DummyRule(&pkg, "a");
  auto b = new DummyRule(&pkg, "b");
  auto c = new DummyRule(&pkg, "c");
  auto d = new DummyRule(&pkg, "d");
  auto e = new DummyRule(&pkg, "e");

  s->Dependencies.push_back(a);

  a->Dependencies.push_back(b);
  a->Dependencies.push_back(c);
  a->Dependencies.push_back(d);

  b->Dependencies.push_back(c);

  d->Dependencies.push_back(e);

  e->Dependencies.push_back(a);

  auto deps = s->DependenciesInOrder();
  auto res = utils::JoinString(" ", deps, [](auto p) {
    return p->Name;
  });

  REQUIRE(res == "s a b c d e a");
  std::cerr << "Deps: " << res << std::endl;
}

}  // namespace jk::core::rules::test

// vim: fdm=marker
