// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <sstream>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/dependent.hh"
#include "jk/core/rules/package.hh"

namespace jk {
namespace core {
namespace rules {

BuildRule::BuildRule(BuildPackage *package, std::string name,
                     std::initializer_list<RuleTypeEnum> types)
    : Package(package), Name(std::move(name)) {
  package->Rules[Name].reset(this);
  for (auto tp : types) {
    Type.SetType(tp);
  }
}

std::string BuildRule::FullQualifiedName() const {
  return fmt::format("{}/{}", Package->Name, Name);
}

void BuildRule::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  dependencies_str_ =
      kwargs.ListOptional("deps", boost::optional<utils::Kwargs::ListType>{{}});
}

void BuildRule::BuildDependencies(BuildPackageFactory *factory,
                                  utils::CollisionNameStack *pstk,
                                  utils::CollisionNameStack *rstk) {
  if (dependencies_has_built_) {
    return;
  }

  if (!rstk->Push(FullQualifiedName())) {
    throw JKBuildError(
        "Build {}'s dependencies failed, this rule has been built before in "
        "this stage. There's a circle: {}",
        FullQualifiedName(), rstk->DumpStack());
  }

  auto ResolveDepends = [this](BuildPackage *pkg, const std::string &name) {
    auto dep_rule = pkg->Rules[name].get();
    if (dep_rule == nullptr) {
      throw JKBuildError("depend on rule: {}:{} but not found!", pkg->Name,
                         name);
    }
    Dependencies.push_back(dep_rule);
  };

  for (const auto &dep_str : dependencies_str_) {
    auto dep_id = ParseIdString(dep_str);

    switch (dep_id.Position) {
      case RuleRelativePosition::kAbsolute: {
        assert(dep_id.PackageName);
        auto dep_pkg = factory->CreatePackage(dep_id.PackageName.get());
        dep_pkg->Initialize(pstk);
        ResolveDepends(dep_pkg, dep_id.RuleName);
      }
      case RuleRelativePosition::kRelative: {
        assert(dep_id.PackageName);

        auto dep_pkg = factory->CreatePackage(
            fmt::format("{}/{}", Package->Name, dep_id.PackageName.get()));
        dep_pkg->Initialize(pstk);
        ResolveDepends(dep_pkg, dep_id.RuleName);
      }
      case RuleRelativePosition::kBuiltin: {
        assert(false);
        throw JKBuildError("not supported");
      }
      case RuleRelativePosition::kThis: {
        ResolveDepends(Package, dep_id.RuleName);
      }
    }
  }

  dependencies_has_built_ = true;
}

}  // namespace rules
}  // namespace core
}  // namespace jk

