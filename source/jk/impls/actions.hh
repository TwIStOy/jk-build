// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <unistd.h>

#include <atomic>
#include <concepts>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/strip.h"
#include "jk/core/executor/script.hh"
#include "jk/core/models/build_package_factory.hh"
#include "jk/core/models/build_rule_factory.hh"
#include "jk/core/models/dependent.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/compiler_factory.hh"
#include "jk/impls/rules/cc_binary.hh"
#include "jk/utils/assert.hh"
#include "jk/utils/logging.hh"
#include "range/v3/algorithm/transform.hpp"
#include "range/v3/range/concepts.hpp"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::impls {

auto CompileRules(
    core::models::Session *session, std::string_view generator_name,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    impls::compilers::CompilerFactory *factory, auto &&rg)
  requires ranges::range<decltype(rg)> &&
           std::same_as<ranges::range_value_t<decltype(rg)>,
                        core::models::BuildRule *>
{
  static auto logger = utils::Logger("generate_all");
  std::vector<std::future<void>> futures;
  for (auto rule : rg) {
    futures.push_back(session->Executor->Push(
        [&scc, generator_name, factory, rule, session]() {
          core::interfaces::Compiler *c =
              factory->Find(generator_name, rule->Base->TypeName);
          if (c != nullptr) {
            logger->debug("Compile {} use {}.{}", rule->Base->StringifyValue,
                          generator_name, rule->Base->TypeName);
            c->Compile(session, scc, rule);
          }
        }));
  }
  return futures;
}

void PrepareRules(core::models::Session *session, auto rg)
  requires ranges::range<decltype(rg)> &&
           std::same_as<ranges::range_value_t<decltype(rg)>,
                        core::models::BuildRule *>
{
  std::vector<std::future<void>> futures;
  for (core::models::BuildRule *rule : rg) {
    futures.push_back(rule->Prepare(session));
  }
  for (auto &f : futures) {
    f.wait();
  }
}

void PrepareDependencies(core::models::Session *session,
                         core::models::BuildPackageFactory *package_factory,
                         auto rules)
  requires ranges::range<decltype(rules)> &&
           std::same_as<ranges::range_value_t<decltype(rules)>,
                        core::models::BuildRule *>
{
  auto dep_str_to_rule = [package_factory](
                             core::models::BuildRule *from,
                             const auto &dep_str) -> core::models::BuildRule * {
    auto dep = core::models::ParseIdString(dep_str);
    switch (dep.Position) {
      case core::models::RuleRelativePosition::kAbsolute: {
        auto pkg = package_factory->PackageUnsafe(*dep.PackageName);
        utils::assertion::boolean.expect(pkg.second == false,
                                         dep.PackageName->c_str());
        auto it = pkg.first->RulesMap.find(dep.RuleName);
        return it->second.get();
      }
      case core::models::RuleRelativePosition::kBuiltin:
        assert(false);
        JK_THROW(core::JKBuildError("not supported"));
        break;
      case core::models::RuleRelativePosition::kRelative: {
        auto next_name =
            fmt::format("{}/{}", from->Package->Name, *dep.PackageName);
        auto pkg = package_factory->PackageUnsafe(*dep.PackageName);
        assert(pkg.second == false);
        auto it = pkg.first->RulesMap.find(dep.RuleName);
        return it->second.get();
      } break;
      case core::models::RuleRelativePosition::kThis: {
        auto it = from->Package->RulesMap.find(dep.RuleName);
        return it->second.get();
      } break;
    }
    return nullptr;
  };

  auto make_dep_str_to_rule = [dep_str_to_rule](core::models::BuildRule *rule) {
    for (const auto &_dep : rule->Base->Dependencies) {
      auto dep = dep_str_to_rule(rule, _dep);
      if (dep != nullptr) {
        rule->Dependencies.push_back(dep);
      }
    }

    if (rule->Base->TypeName == "cc_binary") {
      auto cc_bin = dynamic_cast<rules::CCBinary *>(rule);
      for (const auto &[k, vs] : cc_bin->RawAddtionalDependencies) {
        rules::CCBinary::DependentInBinary dep;
        dep.Rule = dep_str_to_rule(rule, k);
        if (dep.Rule == nullptr) {
          // TODO(hawtian): report error here
          continue;
        }

        for (const auto &v : vs) {
          auto v_rule = dep_str_to_rule(rule, k);
          if (v_rule != nullptr) {
            dep.Dependencies.push_back(v_rule);
          }
        }

        cc_bin->DependenciesInBinary.push_back(std::move(dep));
      }
    }
  };

  auto futures =
      rules |
      ranges::views::transform([session, &make_dep_str_to_rule](auto r) {
        return session->Executor->Push([&make_dep_str_to_rule, r]() {
          make_dep_str_to_rule(r);
        });
      }) |
      ranges::to_vector;
  for (auto &f : futures) {
    f.wait();
  }
}

inline auto LoadBuildFile(core::models::Session *session,
                          core::executor::ScriptInterpreter *interp,
                          std::string_view name,
                          core::models::BuildPackageFactory *package_factory,
                          core::models::BuildRuleFactory *rule_factory)
    -> std::vector<std::string> {
  auto [pkg, new_pkg] = package_factory->Package(name);
  if (!new_pkg) {
    return {};
  }

  pkg->ConstructRules(
      interp->EvalFile(session->Project->Resolve(name, "BUILD").Stringify()),
      rule_factory);

  std::vector<std::string> next_files;
  for (auto rule : pkg->IterRules()) {
    for (const auto &_dep : rule->Base->Dependencies) {
      auto dep = core::models::ParseIdString(_dep);
      switch (dep.Position) {
        case core::models::RuleRelativePosition::kAbsolute:
          next_files.push_back(*dep.PackageName);
          break;
        case core::models::RuleRelativePosition::kBuiltin:
          assert(false);
          JK_THROW(core::JKBuildError("not supported"));
          break;
        case core::models::RuleRelativePosition::kRelative:
          next_files.emplace_back(name);
          break;
        case core::models::RuleRelativePosition::kThis:
          break;
      }
    }
  }

  return std::move(next_files);
}

/*
inline void load_file_impl(std::atomic_uint_fast32_t *pending_jobs,
                           core::models::Session *session,
                           core::models::BuildPackageFactory *package_factory,
                           core::models::BuildRuleFactory *rule_factory,
                           std::string filename) {
  pending_jobs->fetch_add(1);
  session->Executor->Push([pending_jobs, filename, session, package_factory,
                           rule_factory]() mutable {
    auto deps = LoadBuildFile(session, filename, package_factory, rule_factory);

    for (auto &&dep : deps) {
      load_file_impl(pending_jobs, session, package_factory, rule_factory,
                     std::move(dep));
    }

    pending_jobs->fetch_sub(1);
  });
}
*/

template<ranges::range R>
  requires std::convertible_to<ranges::range_value_t<R>, std::string>
void LoadBuildFiles(core::models::Session *session,
                    core::executor::ScriptInterpreter *interp,
                    core::models::BuildPackageFactory *package_factory,
                    core::models::BuildRuleFactory *rule_factory, R files) {
  std::queue<std::string> Q;

  for (const auto &filename : files) {
    Q.push(filename);
  }

  while (!Q.empty()) {
    auto filename = std::move(Q.front());
    Q.pop();

    auto deps =
        LoadBuildFile(session, interp, filename, package_factory, rule_factory);

    for (auto &&dep : deps) {
      Q.push(std::move(dep));
    }
  }
}

}  // namespace jk::impls
