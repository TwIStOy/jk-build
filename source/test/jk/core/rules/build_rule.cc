// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <catch.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "jk/common/path.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/str.hh"

namespace jk::core::rules::test {

struct DummyRule final : public BuildRule {
  DummyRule(BuildPackage *pkg, std::string name)
      : BuildRule(pkg, std::move(name), {RuleTypeEnum::kBinary}, "dummy") {
  }

  std::vector<std::string> ExportedFilesSimpleName(
      filesystem::JKProject *, const std::string &) const final {
    return {};
  }

  std::vector<std::string> ExportedLinkFlags() const final {
    return {};
  }
};

}  // namespace jk::core::rules::test

// vim: fdm=marker
