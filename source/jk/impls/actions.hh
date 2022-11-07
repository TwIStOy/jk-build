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
#include "range/v3/algorithm/transform.hpp"
#include "range/v3/range/concepts.hpp"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::impls {

auto CompileRules(core::models::Session *session,
                  std::string_view generator_name,
                  impls::compilers::CompilerFactory *factory, auto rg)
  requires ranges::range<decltype(rg)> &&
           std::same_as<ranges::range_value_t<decltype(rg)>,
                        core::models::BuildRule *>
{
  std::vector<std::future<void>> futures;
  for (auto rule : rg) {
    futures.push_back(session->Workers.Push([=]() {
      core::interfaces::Compiler *c =
          factory->Find(generator_name, rule->Base->TypeName);
      if (c != nullptr) {
        c->Compile(session, rule);
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
                         core::models::BuildPackageFactory *package, auto rules)
  requires ranges::range<decltype(rules)> &&
           std::same_as<ranges::range_value_t<decltype(rules)>,
                        core::models::BuildRule *>
{
  auto make_dep_str_to_rule = [package](core::models::BuildRule *rule) {
    for (const auto &_dep : *rule->Base->Dependencies) {
      auto dep = core::models::ParseIdString(_dep);
      switch (dep.Position) {
        case core::models::RuleRelativePosition::kAbsolute: {
          auto pkg = package->PackageUnsafe(*dep.PackageName);
          assert(pkg.second == false);
          auto it = pkg.first->RulesMap.find(dep.RuleName);
          rule->Dependencies.emplace_back(it->second.get());
        } break;
        case core::models::RuleRelativePosition::kBuiltin:
          assert(false);
          JK_THROW(core::JKBuildError("not supported"));
          break;
        case core::models::RuleRelativePosition::kRelative: {
          auto next_name =
              fmt::format("{}/{}", rule->Package->Name, *dep.PackageName);
          auto pkg = package->PackageUnsafe(*dep.PackageName);
          assert(pkg.second == false);
          auto it = pkg.first->RulesMap.find(dep.RuleName);
          rule->Dependencies.emplace_back(it->second.get());
        } break;
        case core::models::RuleRelativePosition::kThis: {
          auto it = rule->Package->RulesMap.find(dep.RuleName);
          rule->Dependencies.emplace_back(it->second.get());
        } break;
      }
    }
  };

  rules | ranges::views::transform([session, &make_dep_str_to_rule](auto r) {
    return session->Workers.Push([&make_dep_str_to_rule, r]() {
      make_dep_str_to_rule(r);
    });
  }) | ranges::views::for_each([](auto &f) {
    f.wait();
  });
}

inline auto LoadBuildFile(core::models::Session *session,
                          std::string_view filename,
                          core::models::BuildPackageFactory *package_factory,
                          core::models::BuildRuleFactory *rule_factory)
    -> std::vector<std::string> {
  auto [pkg, new_pkg] = package_factory->Package(filename);
  if (!new_pkg) {
    return {};
  }

  pkg->Name = filename;
  pkg->Path = common::ProjectRelativePath{pkg->Name};
  pkg->ConstructRules(
      core::executor::ScriptInterpreter::ThreadInstance()->EvalFile(filename),
      rule_factory);

  std::vector<std::string> next_files;
  for (auto rule : pkg->IterRules()) {
    for (const auto &_dep : *rule->Base->Dependencies) {
      auto dep = core::models::ParseIdString(_dep);
      switch (dep.Position) {
        case core::models::RuleRelativePosition::kAbsolute:
          next_files.push_back(*dep.PackageName + "/BUILD");
          break;
        case core::models::RuleRelativePosition::kBuiltin:
          assert(false);
          JK_THROW(core::JKBuildError("not supported"));
          break;
        case core::models::RuleRelativePosition::kRelative:
          next_files.emplace_back(absl::StripSuffix(filename, "/BUILD"));
          break;
        case core::models::RuleRelativePosition::kThis:
          break;
      }
    }
  }
  return std::move(next_files);
}

template<ranges::range R>
  requires std::convertible_to<ranges::range_value_t<R>, std::string>
void LoadBuildFiles(core::models::Session *session,
                    core::models::BuildPackageFactory *package_factory,
                    core::models::BuildRuleFactory *rule_factory, R files) {
  std::atomic_uint_fast32_t pending_jobs = 0;

  auto load_file_impl = [&pending_jobs, session, package_factory, rule_factory](
                            std::string filename,
                            auto &&load_file_impl) mutable {
    pending_jobs++;
    session->Workers.Push([&pending_jobs, filename, session, package_factory,
                           rule_factory, load_file_impl]() mutable {
      auto deps =
          LoadBuildFile(session, filename, package_factory, rule_factory);

      for (auto &&dep : deps) {
        load_file_impl(std::move(dep), load_file_impl);
      }

      pending_jobs--;
    });
  };

  for (const auto &filename : files) {
    load_file_impl(filename, load_file_impl);
  }

  while (pending_jobs > 0) {
    usleep(10);
  }
}

}  // namespace jk::impls
