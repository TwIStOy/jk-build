// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/root_compiler.hh"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "jk/cli/cli.hh"
#include "jk/core/algorithms/topological_sort.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/any_view.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/single.hpp"
#include "range/v3/view/transform.hpp"
#include "range/v3/view/unique.hpp"

namespace jk::impls::compilers::makefile {

static auto logger = utils::Logger("makefile.root");

auto RootCompiler::Name() const -> std::string_view {
  return "makefile.root";
}

auto merge_numbers(core::models::BuildRule *rule,
                   absl::flat_hash_set<uint32_t> *visited)
    -> ranges::any_view<uint32_t> {
  if (visited->contains(rule->Base->ObjectId)) {
    return ranges::views::empty<uint32_t>;
  }
  visited->insert(rule->Base->ObjectId);

  return ranges::views::concat(
      rule->Steps.Steps(),
      rule->Dependencies | ranges::views::transform([visited](auto r) {
        return merge_numbers(r, visited);
      }) | ranges::views::join);
}

auto generate_targets(core::models::Session *session,
                      core::generators::Makefile *makefile,
                      std::vector<std::string> *clean_targets,
                      std::vector<std::string> *test_targets,
                      core::models::BuildRule *rule) {
  absl::flat_hash_set<uint32_t> visited;
  auto numbers = merge_numbers(rule, &visited) | ranges::to_vector;

  // add clean target
  if (rule->Base->Type.IsExternal()) {
    auto clean_target = rule->WorkingFolder.Sub("clean").Stringify();
    makefile->Target(
        clean_target, ranges::views::empty<std::string>,
        core::builder::CustomCommandLines::Single(
            {"@$(MAKE)", "-f",
             rule->WorkingFolder.Sub("build.make").Stringify(), "clean"}),
        "", true);
    clean_targets->push_back(std::move(clean_target));
  }

  // add test target
  if (rule->Base->Type.IsCC() && rule->Base->Type.IsTest()) {
    auto test_target = rule->WorkingFolder.Sub("test").Stringify();
    makefile->Target(
        test_target, ranges::views::empty<std::string>,
        core::builder::CustomCommandLines::Single(
            {"@$(MAKE)", "-f",
             rule->WorkingFolder.Sub("build.make").Stringify(), "test"}),
        "", true);
    test_targets->push_back(std::move(test_target));
  }

  if (rule->Base->Type.IsCC()) {
    // cc target has build type
    for (const auto &build_type : session->BuildTypes) {
      auto deps = ranges::views::concat(
          rule->Dependencies |
              ranges::views::transform([&build_type](auto rule) {
                if (rule->Base->Type.IsCC()) {
                  return fmt::format("{}/{}", rule->Base->FullQualifiedName,
                                     build_type);
                } else {
                  return fmt::format("{}/build", rule->Base->FullQualifiedName);
                }
              }),
          ranges::views::single("pre"));

      auto name = [&build_type, rule]() {
        if (rule->Base->Type.IsCC()) {
          return fmt::format("{}/{}", rule->Base->FullQualifiedName,
                             build_type);
        } else {
          return fmt::format("{}/build", rule->Base->FullQualifiedName);
        }
      }();

      auto cmds = core::builder::CustomCommandLines::Multiple(
          core::builder::CustomCommandLine::Make(
              {"@$(MAKE)", "-f",
               rule->WorkingFolder.Sub("build.make").Stringify(), build_type}),
          PrintStatement(session->Project.get(), "", false, numbers,
                         "Built rule <cyan>{}:{}</cyan>, artifact: [{}]",
                         rule->Package->Name, rule->Base->Name,
                         absl::StrJoin(rule->ExportedFiles(session, build_type),
                                       ", ", [](std::string *out, auto &s) {
                                         out->append("<green>");
                                         out->append(s);
                                         out->append("</green>");
                                       })));

      makefile->Target(name, deps, cmds);
    }
  } else {
    // simple target
    auto deps = ranges::views::concat(
        rule->Dependencies | ranges::views::transform([](auto rule) {
          if (!rule->Base->Type.IsExternal()) {
            JK_THROW(core::JKBuildError(
                "ExternalProject can only depend on another ExternalProject."));
          }
          return fmt::format("{}/build", rule->Base->FullQualifiedName);
        }),
        ranges::views::single("pre"));

    auto cmds = core::builder::CustomCommandLines::Multiple(
        core::builder::CustomCommandLine::Make(
            {"@$(MAKE)", "-f",
             rule->WorkingFolder.Sub("build.make").Stringify(), "build"}),
        PrintStatement(session->Project.get(), "", false, numbers,
                       "Built rule <cyan>{}:{}</cyan>", rule->Package->Name,
                       rule->Base->Name));

    auto name = fmt::format("{}/build", rule->Base->FullQualifiedName);
    makefile->Target(name, deps, cmds, "", true);
    makefile->Target("external", ranges::views::single(name),
                     ranges::views::empty<core::builder::CustomCommandLine>);
  }
}

auto RootCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    std::vector<core::models::BuildRule *> rules) const -> void {
  auto makefile = new_makefile_with_common_commands(
      session, session->Project->ProjectRoot, "Makefile", true);

  makefile.Target("all", ranges::views::single("debug"),
                  ranges::views::empty<core::builder::CustomCommandLine>);

  makefile.Target(core::generators::Makefile::DEFAULT_TARGET,
                  ranges::views::single("all"),
                  ranges::views::empty<core::builder::CustomCommandLine>);

  auto regen_target =
      session->Project->BuildRoot.Sub("build_files.mark").Stringify();

  makefile.Target(
      "pre", ranges::views::single(regen_target),
      core::builder::CustomCommandLines::Multiple(
          core::builder::CustomCommandLine::Make(
              {"@$(JK_COMMAND)", "start_progress",
               fmt::format("--progress-mark={}",
                           session->Project->BuildRoot.Sub("progress.mark")
                               .Stringify()),
               fmt::format("--progress-dir={}",
                           session->Project->BuildRoot.Stringify())}),
          // backward compatibility
          core::builder::CustomCommandLine::Make(
              {"mkdir", "-p",
               session->Project->ProjectRoot.Sub(".build", "pb", "c++")
                   .Stringify()}),
          core::builder::CustomCommandLine::Make(
              {"mkdir", "-p",
               session->Project->ProjectRoot
                   .Sub(".build", ".lib", "m64", "include")
                   .Stringify()})),
      "", true);

  makefile.Target("external", ranges::views::empty<std::string>,
                  ranges::views::empty<core::builder::CustomCommandLine>, "",
                  true);

  auto sorted = core::algorithms::topological_sort(scc);
  absl::flat_hash_set<uint32_t> visited;
  auto dfs = [&](uint32_t id, auto &&dfs) -> void {
    visited.insert(id);
    for (uint32_t n : scc[id].Deps) {
      visited.insert(n);
      dfs(n, dfs);
    }
  };
  for (auto r : rules) {
    dfs(r->_scc_id, dfs);
  }
  auto all_rules =
      sorted | ranges::views::filter([&](auto id) {
        return visited.contains(id);
      }) |
      ranges::views::for_each([&](auto id) {
        return ranges::yield_from(ranges::views::all(scc[id].Rules));
      }) |
      ranges::to_vector;

  std::vector<std::string> clean_targets;
  std::vector<std::string> test_targets;
  clean_targets.reserve(all_rules.size());
  test_targets.reserve(all_rules.size());

  auto packages = all_rules |
                  ranges::views::transform([&](core::models::BuildRule *rule) {
                    return rule->Package->Name;
                  }) |
                  ranges::to<absl::flat_hash_set<std::string>>;

  auto regen_stmt = core::builder::CustomCommandLine::FromVec(
      ranges::views::concat(ranges::views::single("-@$(JK_COMMAND)"),
                            cli::CommandLineArguments) |
      ranges::to_vector);
  auto regen_touch_stmt =
      core::builder::CustomCommandLine::Make({"@touch", regen_target});

  makefile.Target(
      regen_target,
      ranges::views::concat(
          packages | ranges::views::transform([&](const auto &s) {
            return session->Project->ProjectRoot.Sub(s, "BUILD").Stringify();
          }),
          ranges::views::single(
              session->Project->ProjectRoot.Sub(session->ProjectMarker)
                  .Stringify())),
      core::builder::CustomCommandLines::Multiple(regen_stmt,
                                                  regen_touch_stmt));

  for (auto r : all_rules) {
    generate_targets(session, &makefile, &clean_targets, &test_targets, r);
  }

  for (auto rule : all_rules) {
    for (const auto &build_type : session->BuildTypes) {
      std::string tgt_name;
      if (rule->Base->Type.IsCC()) {
        tgt_name =
            fmt::format("{}/{}", rule->Base->FullQualifiedName, build_type);
      } else {
        tgt_name = fmt::format("{}/build", rule->Base->FullQualifiedName);
      }
      makefile.Target(build_type,
                      ranges::views::concat(ranges::views::single(tgt_name),
                                            ranges::views::single("pre")),
                      ranges::views::empty<core::builder::CustomCommandLine>,
                      "", true);
    }
  }

  for (const auto &build_type : session->BuildTypes) {
    makefile.Target(
        absl::AsciiStrToLower(build_type), ranges::views::single(build_type),
        ranges::views::empty<core::builder::CustomCommandLine>, "simple usage");
  }
  makefile.Target("clean", clean_targets,
                  ranges::views::empty<core::builder::CustomCommandLine>, "",
                  true);
  makefile.Target("test", test_targets,
                  ranges::views::empty<core::builder::CustomCommandLine>, "",
                  true);
}

}  // namespace jk::impls::compilers::makefile
