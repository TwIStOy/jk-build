// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/dependent.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

std::string RuleType::Stringify() const {
  std::vector<std::string> flags;
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kLibrary)) {
    flags.push_back("library");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kBinary)) {
    flags.push_back("binary");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kTest)) {
    flags.push_back("test");
  }
  return "RuleType [{}]"_format(
      utils::JoinString("|", flags.begin(), flags.end()));
}

std::string BuildRule::Stringify() const {
  std::ostringstream oss;
  oss << "BuildRule { ";

  std::vector<std::string> fields;

  fields.push_back("Package = {}"_format(Package->Name));
  fields.push_back("Name = {}"_format(Name));
  fields.push_back("Type = {}"_format(Type));
  fields.push_back("TypeName = {}"_format(TypeName));
  fields.push_back("Dependencies = [{}]"_format(utils::JoinString(
      ", ", dependencies_str_.begin(), dependencies_str_.end())));

  oss << utils::JoinString(", ", fields.begin(), fields.end());

  oss << " }";
  return oss.str();
}

RuleType MergeType(std::initializer_list<RuleTypeEnum> types) {
  RuleType rtp;
  for (auto tp : types) {
    rtp.SetType(tp);
  }
  return rtp;
}

BuildRule::BuildRule(BuildPackage *package, std::string name,
                     std::initializer_list<RuleTypeEnum> types,
                     std::string_view type_name)
    : Package(package),
      Name(std::move(name)),
      Type(MergeType(types)),
      TypeName(type_name) {
  package->Rules[Name].reset(this);
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
    JK_THROW(JKBuildError(
        "Build {}'s dependencies failed, this rule has been built before in "
        "this stage. There's a circle: {}",
        FullQualifiedName(), rstk->DumpStack()));
  }

  auto ResolveDepends = [this](BuildPackage *pkg, const std::string &name) {
    auto dep_rule = pkg->Rules[name].get();
    if (dep_rule == nullptr) {
      JK_THROW(JKBuildError("depend on rule: {}:{} but not found!", pkg->Name,
                            name));
    }
    Dependencies.push_back(dep_rule);
  };

  for (const auto &dep_str : dependencies_str_) {
    auto dep_id = ParseIdString(dep_str);

    switch (dep_id.Position) {
      case RuleRelativePosition::kAbsolute: {
        assert(dep_id.PackageName);
        auto dep_pkg = factory->Package(dep_id.PackageName.get());
        dep_pkg->Initialize(pstk);
        ResolveDepends(dep_pkg, dep_id.RuleName);
      }
      case RuleRelativePosition::kRelative: {
        assert(dep_id.PackageName);

        auto dep_pkg = factory->Package(
            fmt::format("{}/{}", Package->Name, dep_id.PackageName.get()));
        dep_pkg->Initialize(pstk);
        ResolveDepends(dep_pkg, dep_id.RuleName);
      }
      case RuleRelativePosition::kBuiltin: {
        assert(false);
        JK_THROW(JKBuildError("not supported"));
      }
      case RuleRelativePosition::kThis: {
        ResolveDepends(Package, dep_id.RuleName);
      }
    }
  }

  rstk->Pop();

  dependencies_has_built_ = true;
}

}  // namespace rules
}  // namespace core
}  // namespace jk

