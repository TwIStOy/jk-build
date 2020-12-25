// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <chrono>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "jk/common/path.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/utils/stack.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

using BuildRuleMap =
    std::unordered_map<std::string, std::unique_ptr<BuildRule>>;
using BuildPackageMap =
    std::unordered_map<std::string, std::unique_ptr<BuildPackage>>;

struct BuildPackage : public utils::Stringifiable {
  BuildPackage(std::string name, common::ProjectRelativePath path);

  //! A package's name is its relative path from project root.
  std::string Name;

  //! All rules in this package.
  BuildRuleMap Rules;

  common::ProjectRelativePath Path;

  //! Initialize package. Note that a package can not be initialized twice, the
  //! second call will not take effect.
  void Initialize(utils::CollisionNameStack *stk);

  std::string Stringify() const final;

  std::chrono::system_clock LastModified() const;

 private:
  bool initialized_ = false;
};

class BuildPackageFactory {
 public:
  //! Get a *BuildPackage* from its address. This function will assume that
  //! any two calls with same argument will return the a same object.
  BuildPackage *Package(const std::string &name);

 private:
  BuildPackageMap packages_;
};

}  // namespace rules
}  // namespace core
}  // namespace jk
