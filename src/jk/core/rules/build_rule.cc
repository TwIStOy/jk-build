// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <queue>
#include <set>
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
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

static auto logger = utils::Logger("BuildRule");

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
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kExternal)) {
    flags.push_back("external");
  }
  if (value_ & static_cast<uint8_t>(RuleTypeEnum::kCC)) {
    flags.push_back("cc");
  }
  return "RuleType [{}]"_format(
      utils::JoinString("|", flags.begin(), flags.end()));
}

std::list<BuildRule const *> BuildRule::DependenciesInOrder() const {
  if (topological_sorting_result_) {
    return topological_sorting_result_.value();
  }

  struct TopItem {
    BuildRule const *Rule{nullptr};
    uint32_t RestOutgoing{0};
    bool Built{false};

    std::list<BuildRule const *> Incoming{};
  };

  using TopItemMap =
      std::unordered_map<std::string /* FullQualifiedName */, TopItem>;

  std::function<void(BuildRule const *, TopItemMap *)> dfs;
  dfs = [&dfs](BuildRule const *rule, TopItemMap *mp) {
    auto &item = (*mp)[rule->FullQualifiedName()];
    if (item.Built) {
      return;
    }
    item.Rule = rule;
    item.RestOutgoing = rule->Dependencies.size();
    item.Built = true;

    for (const auto &dep : rule->Dependencies) {
      (*mp)[dep->FullQualifiedName()].Incoming.push_back(rule);
      dfs(dep, mp);
    }
  };

  TopItemMap items;
  dfs(this, &items);

  std::queue<BuildRule const *> Q;
  for (const auto &[name, item] : items) {
    if (item.RestOutgoing <= 0) {
      Q.push(item.Rule);
    }
  }

  if (Q.empty()) {
    JK_THROW(JKBuildError("unexpected circular dependency"));
  }

  std::list<BuildRule const *> after_sorted;
  while (!Q.empty()) {
    auto rule = Q.front();
    Q.pop();
    if (rule != this) {
      after_sorted.push_back(rule);
    }

    auto &item = items[rule->FullQualifiedName()];
    for (const auto &incoming : item.Incoming) {
      auto &incoming_item = items[incoming->FullQualifiedName()];
      incoming_item.RestOutgoing--;
      if (incoming_item.RestOutgoing <= 0) {
        Q.push(incoming);
      }
    }
  }

  if (after_sorted.size() + 1 /* this */ != items.size()) {
    JK_THROW(JKBuildError("unexpected circular dependency"));
  }

  std::reverse(std::begin(after_sorted), std::end(after_sorted));
  topological_sorting_result_ = after_sorted;
  logger->info(
      "after topological sort: [{}]",
      utils::JoinString(", ", after_sorted, [](const BuildRule *const rule) {
        return rule->FullQualifiedName();
      }));
  return after_sorted;
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

std::string BuildRule::FullQualifiedTarget(const std::string &output) const {
  if (Type.IsCC()) {
    return fmt::format("{}/{}", FullQualifiedName(), output);
  } else {
    return fmt::format("{}/build", FullQualifiedName());
  }
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

  logger->info("<Rule: {}> build dependencies", FullQualifiedName());

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
    return dep_rule;
  };

  for (const auto &dep_str : dependencies_str_) {
    auto dep_id = ParseIdString(dep_str);
    BuildRule *dep = nullptr;

    switch (dep_id.Position) {
      case RuleRelativePosition::kAbsolute: {
        assert(dep_id.PackageName);
        auto dep_pkg = factory->Package(dep_id.PackageName.get());
        dep_pkg->Initialize(pstk);
        dep = ResolveDepends(dep_pkg, dep_id.RuleName);
      } break;
      case RuleRelativePosition::kRelative: {
        assert(dep_id.PackageName);

        auto dep_pkg = factory->Package(
            fmt::format("{}/{}", Package->Name, dep_id.PackageName.get()));
        dep_pkg->Initialize(pstk);
        dep = ResolveDepends(dep_pkg, dep_id.RuleName);
      } break;
      case RuleRelativePosition::kBuiltin: {
        assert(false);
        JK_THROW(JKBuildError("not supported"));
      } break;
      case RuleRelativePosition::kThis: {
        dep = ResolveDepends(Package, dep_id.RuleName);
      } break;
    }

    dep->BuildDependencies(factory, pstk, rstk);
  }

  rstk->Pop();

  dependencies_has_built_ = true;
}

common::AbsolutePath BuildRule::WorkingFolder(
    const common::AbsolutePath &build_root) const {
  return build_root.Sub(utils::Replace(FullQualifiedName(), '/', "@"));
}

void BuildRule::RecursiveExecute(std::function<void(BuildRule *)> func,
                                 std::unordered_set<std::string> *recorder) {
  if (recorder) {
    auto it = recorder->find(this->FullQualifiedName());
    if (it != recorder->end()) {
      return;
    }
    recorder->insert(this->FullQualifiedName());
  }

  func(this);

  for (auto it : Dependencies) {
    it->RecursiveExecute(func, recorder);
  }
}

json BuildRule::CacheState() const {
  json res;

  res["name"] = FullQualifiedName();
  res["package"] = Package->Name;
  res["type_name"] = TypeName;
  std::vector<std::string> deps;
  std::transform(std::begin(Dependencies), std::end(Dependencies),
                 std::back_inserter(deps), [](BuildRule *r) {
                   return r->FullQualifiedName();
                 });
  res["deps"] = deps;

  return res;
}

}  // namespace rules
}  // namespace core
}  // namespace jk

