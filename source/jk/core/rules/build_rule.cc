// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <list>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/common/counter.hh"
#include "jk/core/constant.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/dependent.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

static auto logger = utils::Logger("rule");

std::string RuleType::Stringify() const {  // {{{
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
      utils::JoinString(" | ", flags.begin(), flags.end()));
}  // }}}

std::list<BuildRule const *> BuildRule::DependenciesInOrder() const {  // {{{
  if (deps_sorted_list_) {
    return deps_sorted_list_.value();
  }

  std::unordered_set<std::string> occurrences;

  using RuleList = std::list<BuildRule const *>;
  RuleList result;
  std::unordered_map<std::string, std::unordered_set<std::string>> deps;
  std::unordered_map<std::string, std::unordered_set<std::string>> be_deps;

  // extend *result* by inserting deps
  std::function<void(BuildRule const *)> dfs;
  dfs = [&occurrences, &result, &dfs, &deps, &be_deps](BuildRule const *rule) {
    if (auto it = occurrences.find(rule->FullQualifiedName());
        it != occurrences.end()) {
      result.push_back(rule);
      return;
    }

    for (auto dep : rule->Dependencies) {
      deps[rule->FullQualifiedName()].insert(dep->FullQualifiedName());
      be_deps[dep->FullQualifiedName()].insert(rule->FullQualifiedName());
    }

    occurrences.insert(rule->FullQualifiedName());
    result.push_back(rule);
    for (auto dep : rule->Dependencies) {
      dfs(dep);
    }
  };

  dfs(this);

  // try to remove duplicate rule

  // auto backward_dup = [&result](RuleList::iterator now) {
  //   if (now == result.begin()) {
  //     return false;
  //   }
  //   auto it = now;
  //   --it;
  //
  //   return false;
  // };
  //
  // auto forward_dup = []() {
  // };

  // for (auto left = result.begin(); left != result.end();) {
  //   bool has_dep = false;
  //   bool only = true;
  //   auto &left_deps = (*left)->Dependencies;
  //
  //   for (auto right = std::next(left); right != result.end(); ++right) {
  //     if (*right == *left) {
  //       only = false;
  //       break;
  //     }
  //
  //     if (std::find(std::begin(left_deps), std::end(left_deps), *right) !=
  //         std::end(left_deps)) {
  //       has_dep = true;
  //     }
  //   }
  //
  //   if (!has_dep && !only) {
  //     left = result.erase(left);
  //   } else {
  //     ++left;
  //   }
  // }

  deps_sorted_list_ = result;
  return deps_sorted_list_.value();
}  // }}}

std::string BuildRule::Stringify() const {  // {{{
  return R"(<Rule:{} "{}:{}">)"_format(TypeName, Package->Name, Name);
}  // }}}

RuleType MergeType(std::initializer_list<RuleTypeEnum> types) {  // {{{
  RuleType rtp;
  for (auto tp : types) {
    rtp.SetType(tp);
  }
  return rtp;
}  // }}}

BuildRule::BuildRule(BuildPackage *package, std::string name,  // {{{
                     std::initializer_list<RuleTypeEnum> types,
                     std::string_view type_name)
    : Package(package),
      Name(std::move(name)),
      Type(MergeType(types)),
      TypeName(type_name) {
  package->Rules[Name].reset(this);
}  // }}}

std::string BuildRule::FullQualifiedName() const {  // {{{
  return fmt::format("{}/{}", Package->Name, Name);
}  // }}}

std::string BuildRule::FullQuotedQualifiedName() const {  // {{{
  auto str = FullQualifiedName();
  for (auto &ch : str) {
    if (ch == '/') {
      ch = '_';
    }
  }
  return str;
}  // }}}

std::string BuildRule::FullQualifiedTarget(  // {{{
    const std::string &output) const {
  if (Type.IsCC()) {
    return fmt::format("{}/{}", FullQualifiedName(), output);
  } else {
    return fmt::format("{}/build", FullQualifiedName());
  }
}  // }}}

void BuildRule::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {  // {{{
  dependencies_str_ =
      kwargs.ListOptional("deps", boost::optional<utils::Kwargs::ListType>{{}});
}  // }}}

void BuildRule::BuildDependencies(  // {{{
    filesystem::JKProject *project, BuildPackageFactory *factory) {
  if (dependencies_built_state_ == InitializeState::kProcessing ||
      dependencies_built_state_ == InitializeState::kDone) {
    return;
  }

  dependencies_built_state_ = InitializeState::kProcessing;

  logger->debug("{} build dependencies", *this);

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
        dep_pkg->Initialize(project);
        dep = ResolveDepends(dep_pkg, dep_id.RuleName);
      } break;
      case RuleRelativePosition::kRelative: {
        assert(dep_id.PackageName);

        auto dep_pkg = factory->Package(
            fmt::format("{}/{}", Package->Name, dep_id.PackageName.get()));
        dep_pkg->Initialize(project);
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

    dep->BuildDependencies(project, factory);
  }

  dependencies_built_state_ = InitializeState::kDone;
}  // }}}

common::AbsolutePath BuildRule::WorkingFolder(  // {{{
    const common::AbsolutePath &build_root) const {
  return build_root.Sub(utils::Replace(FullQualifiedName(), '/', "@"));
}  // }}}

void BuildRule::RecursiveExecute(std::function<void(BuildRule *)> func,  // {{{
                                 std::unordered_set<std::string> *recorder) {
  if (recorder) {
    if (auto it = recorder->find(this->FullQualifiedName());
        it != recorder->end()) {
      return;
    }
    recorder->insert(this->FullQualifiedName());
  }

  for (auto it : Dependencies) {
    it->RecursiveExecute(func, recorder);
  }

  func(this);
}  // }}}

json BuildRule::CacheState() const {  // {{{
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
}  // }}}

std::unordered_map<std::string, std::string>  // {{{
BuildRule::ExportedEnvironmentVar(filesystem::JKProject *project) const {
  (void)project;
  return {};
}  // }}}

uint32_t BuildRule::KeyNumber(const std::string &key) {  // {{{
  if (auto it = key_numbers_.find(key); it != key_numbers_.end()) {
    return it->second;
  }
  auto id = common::Counter()->Next();
  key_numbers_[key] = id;
  return id;
}  // }}}

std::vector<uint32_t> BuildRule::KeyNumbers() const {  //{{{
  std::vector<uint32_t> res;
  std::transform(std::begin(key_numbers_), std::end(key_numbers_),
                 std::back_inserter(res), [](const auto &p) -> uint32_t {
                   return p.second;
                 });
  return res;
}  // }}}

}  // namespace rules
}  // namespace core
}  // namespace jk

// vim: fdm=marker
