// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/root_compiler.hh"

#include "jk/core/algorithms/topological_sort.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

static auto logger = utils::Logger("makefile.root");

auto RootCompiler::Name() const -> std::string_view {
  return "makefile.root";
}

auto RootCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    std::vector<core::models::BuildRule *> rules) const -> void {
  auto makefile =
      new_makefile_with_common_commands(session, session->Project->ProjectRoot);

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
               "--progress-mark={}"_format(
                   session->Project->BuildRoot.Sub("progress.mark")
                       .Stringify()),
               "--progress-dir={}"_format(
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

  std::list<std::string> clean_targets;
  std::list<std::string> test_targets;

  auto sorted = core::algorithms::topological_sort(scc);

  std::unordered_set<std::string> recorder;
  /*
   * auto gen_target = [&makefile, project, &recorder, &clean_targets,
   *                    &test_targets](rules::BuildRule *rule) {
   *   if (recorder.find(rule->FullQualifiedName()) != recorder.end()) {
   *     return;
   *   }
   *   recorder.insert(rule->FullQualifiedName());
   *
   *   logger->info("Generate global target for {}, type: {}", *rule,
   * rule->Type);
   *
   *   auto working_folder = rule->WorkingFolder(project->BuildRoot);
   *
   *   std::unordered_set<uint32_t> numbers;
   *   auto merge_numbers = [&](rules::BuildRule *rule) {
   *     for (auto id : rule->KeyNumbers()) {
   *       numbers.insert(id);
   *     }
   *   };
   *   std::unordered_set<std::string> _recorder;
   *   rule->RecursiveExecute(merge_numbers, &_recorder);
   *   _recorder.clear();
   *
   *   if (!rule->Type.IsExternal()) {
   *     auto clean_target = working_folder.Sub("clean");
   *     makefile->AddTarget(
   *         clean_target, {},
   *         builder::CustomCommandLines::Single(
   *             {"@$(MAKE)", "-f",
   * working_folder.Sub("build.make").Stringify(), "clean"}),
   *         "", true);
   *     clean_targets.push_back(clean_target);
   *   }
   *
   *   if (rule->Type.IsCC() && rule->Type.IsTest()) {
   *     auto test_target = working_folder.Sub("test");
   *     makefile->AddTarget(
   *         test_target, {},
   *         builder::CustomCommandLines::Single(
   *             {"@$(MAKE)", "-f",
   * working_folder.Sub("build.make").Stringify(), "test"}),
   *         "", true);
   *     test_targets.push_back(test_target);
   *   }
   *
   *   if (rule->Type.IsCC()) {
   *     // cc target has build type
   *     for (const auto &output_format : formats) {
   *       std::list<std::string> deps;
   *       for (auto dep : rule->Dependencies) {
   *         deps.push_back(dep->FullQualifiedTarget(output_format.first));
   *       }
   *       deps.push_back("pre");
   *
   *       makefile->AddTarget(
   *           rule->FullQualifiedTarget(output_format.first), deps,
   *           builder::CustomCommandLines::Multiple(
   *               builder::CustomCommandLine::Make(
   *                   {"@$(MAKE)", "-f",
   *                    working_folder.Sub("build.make").Stringify(),
   *                    output_format.second}),
   *               ::jk::rules::PrintPlain(
   *                   project, numbers,
   *                   "Built rule <cyan>{}:{}</cyan>, artifact: "
   *                   "[{}]",
   *                   rule->Package->Name, rule->Name,
   *                   utils::JoinString(", ",
   *                                     rule->ExportedFilesSimpleName(
   *                                         project, output_format.second),
   *                                     [](const auto &s) {
   *                                       return
   * fmt::format("<green>{}</green>", s);
   *                                     }))));
   *     }
   *   } else {
   *     // simple target
   *     std::list<std::string> deps;
   *     for (auto dep : rule->Dependencies) {
   *       if (!dep->Type.IsExternal()) {
   *         JK_THROW(JKBuildError(
   *             "ExternalProject can only depend on another
   * ExternalProject."));
   *       }
   *       deps.push_back(dep->FullQualifiedTarget());
   *     }
   *     deps.push_back("pre");
   *
   *     auto print_stmt = ::jk::rules::PrintPlain(
   *         project, numbers, "Built rule <cyan>{}:{}</cyan>",
   *         rule->Package->Name, rule->Name);
   *
   *     makefile->AddTarget(
   *         rule->FullQualifiedTarget(), deps,
   *         builder::CustomCommandLines::Multiple(
   *             builder::CustomCommandLine::Make(
   *                 {"@$(MAKE)", "-f",
   *                  working_folder.Sub("build.make").Stringify(), "build"}),
   *             print_stmt),
   *         "", true);
   *     makefile->AddTarget("external", {rule->FullQualifiedTarget()});
   *   }
   * };
   */

  /*
   * std::unordered_set<std::string> packages;
   * for (auto rule : rules) {
   *   packages.insert("{}/BUILD"_format(rule->Package->Name));
   *   auto deps = rule->DependenciesAlwaysBehind();
   *   for (auto dep : deps) {
   *     packages.insert("{}/BUILD"_format(dep->Package->Name));
   *   }
   * }
   *
   * auto regen_stmt = builder::CustomCommandLine::Make({"-@$(JK_COMMAND)"});
   * std::copy(std::next(std::begin(cli::CommandLineArguments)),
   *           std::end(cli::CommandLineArguments),
   *           std::back_inserter(regen_stmt));
   * auto regen_touch_stmt =
   *     builder::CustomCommandLine::Make({"@touch", regen_target});
   *
   * std::list<std::string> regen_deps{std::begin(packages),
   * std::end(packages)}; regen_deps.push_back((project->ProjectRoot.Path /
   * "JK_ROOT").string()); makefile->AddTarget( regen_target, regen_deps,
   *     builder::CustomCommandLines::Multiple(regen_stmt, regen_touch_stmt));
   *
   * for (auto rule : rules) {
   *   std::unordered_set<std::string> recorder;
   *   rule->RecursiveExecute(gen_target, &recorder);
   *
   *   for (const auto &output_format : formats) {
   *     auto tgt_name =
   *         fmt::format("{}/{}", rule->FullQualifiedName(),
   * output_format.first); makefile->AddTarget(output_format.first, {tgt_name,
   * "pre"}, {}, "", true);
   *   }
   * }
   *
   * makefile->AddTarget("clean", clean_targets, {}, "", true);
   * makefile->AddTarget("test", test_targets, {}, "", true);
   *
   * auto w = wf->Build(makefile->filename_);
   * makefile->Write(w.get());
   */
}

}  // namespace jk::impls::compilers::makefile
