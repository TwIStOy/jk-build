// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <atomic>
#include <concepts>
#include <string_view>

#include "jk/core/algorithms/tarjan.hh"
#include "jk/core/models/build_package_factory.hh"
#include "jk/core/models/build_rule_base.hh"
#include "jk/core/models/build_rule_factory.hh"
#include "jk/core/models/dependent.hh"
#include "jk/core/models/helpers.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/actions.hh"
#include "jk/utils/assert.hh"
#include "jk/version.h"
#include "range/v3/range/concepts.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/any_view.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/single.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::impls::actions {

static auto release_git_version_file_content = R"(
// Generated by JK, JK Version )" JK_VERSION
                                               R"(//

#pragma once  // NOLINT(build/header_guard)

#if !defined(BUILD_TIME)
#define BUILD_TIME __DATE__ " "  __TIME__
#endif
)";

auto generate_all(core::models::Session *session,
                  core::executor::ScriptInterpreter *interp,
                  auto generator_names,
                  impls::compilers::CompilerFactory *compiler_factory,
                  core::models::BuildPackageFactory *package_factory,
                  core::models::BuildRuleFactory *rule_factory, auto &&rg)
  requires ranges::range<decltype(rg)> &&
           std::same_as<ranges::range_value_t<decltype(rg)>,
                        core::models::BuildRuleId>
{
  LoadBuildFiles(session, interp, package_factory, rule_factory,
                 rg | ranges::views::transform([](auto &id) -> decltype(auto) {
                   return *id.PackageName;
                 }));

  PrepareDependencies(session, package_factory,
                      core::models::IterAllRules(package_factory));

  PrepareRules(session, core::models::IterAllRules(package_factory));

  auto arg_rules =
      rg |
      ranges::views::transform(
          [&](core::models::BuildRuleId &id)
              -> ranges::any_view<core::models::BuildRule *> {
            auto [pkg, new_pkg] =
                package_factory->PackageUnsafe(*id.PackageName);
            fmt::print("unsafe: {}\n", *id.PackageName);
            utils::assertion::boolean.expect(!new_pkg, id.PackageName->c_str());

            if (id.RuleName == "...") {
              // "..." means all rules
              return pkg->IterRules();
            } else {
              auto rule = pkg->RulesMap[id.RuleName].get();
              if (!rule) {
                JK_THROW(
                    core::JKBuildError("No rule named '{}' in package '{}'",
                                       id.RuleName, id.PackageName.value()));
              }
              return ranges::views::single(rule);
            }
          }) |
      ranges::views::join | ranges::to_vector;

  auto scc = core::algorithms::Tarjan(session, ranges::views::all(arg_rules));

  std::vector<bool> visited(core::models::__CurrentObjectId(), false);
  std::vector<core::models::BuildRule *> all_rules;
  auto dfs = [&visited, &all_rules](core::models::BuildRule *r, auto &&dfs) {
    if (visited[r->Base->ObjectId]) {
      return;
    }
    all_rules.push_back(r);
    for (auto n : r->Dependencies) {
      dfs(n, dfs);
    }
  };

  std::vector<std::future<void>> futures;
  for (auto generator_name : generator_names) {
    auto res = CompileRules(session, generator_name, scc, compiler_factory,
                            ranges::views::all(all_rules));
    for (auto &&f : res) {
      futures.push_back(std::move(f));
    }
  }

  for (auto &f : futures) {
    f.wait();
  }

  // generate progress.mark
  {
    std::ofstream ofs(
        session->Project->BuildRoot.Sub("progress.mark").Stringify());
    if (ofs) {
      ofs << common::Counter()->Now();
    } else {
      JK_THROW(
          core::JKBuildError("Could not write progress count file to {}.",
                             session->Project->BuildRoot.Sub("progress.mark")));
    }
  }

  // generate 'release/git_version.h'
  {
    auto folder =
        session->Project->ProjectRoot.Sub(".build", "include", "release");
    common::AssumeFolder(folder.Path);
    std::ofstream ofs(folder.Sub("git_version.h").Stringify());
    ofs << release_git_version_file_content;
    ofs.flush();
  }

  return scc;
}

}  // namespace jk::impls::actions
