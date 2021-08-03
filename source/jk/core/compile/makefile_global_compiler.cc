// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/makefile_global_compiler.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "jk/cli/cli.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/error.h"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/rules/cc/compiler/cc_binary.hh"
#include "jk/rules/cc/compiler/cc_library.hh"
#include "jk/rules/cc/compiler/cc_test.hh"
#include "jk/rules/cc/compiler/proto_library.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/rules/common.hh"
#include "jk/rules/external/compiler/cmake_project.hh"
#include "jk/rules/external/compiler/external_library.hh"
#include "jk/rules/external/compiler/shell_script.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "spdlog/spdlog.h"

namespace jk::core::compile {

static auto logger = utils::Logger("makefile_global_compiler");

static std::vector<std::pair<std::string, std::string>> formats = {
    {"debug", "DEBUG"}, {"release", "RELEASE"}, {"profiling", "PROFILING"}};

void MakefileGlobalCompiler::Compile(
    filesystem::JKProject *project, writer::WriterFactory *wf,
    const std::vector<rules::BuildRule *> &rules) {
  output::UnixMakefilePtr makefile{new output::UnixMakefile(
      project->ProjectRoot.Sub("Makefile").Stringify())};

  auto regen_target = project->BuildRoot.Sub("build_files.mark").Stringify();

  makefile->DefineCommon(project);
  makefile->AddTarget("all", {"debug"}, {}, "", true);
  makefile->DefaultTarget("all");

  makefile->AddTarget(
      "pre", {regen_target},
      builder::CustomCommandLines::Multiple(
          builder::CustomCommandLine::Make(
              {"@$(JK_COMMAND)", "start_progress",
               "--progress-mark={}"_format(
                   project->BuildRoot.Sub("progress.mark")),
               "--progress-dir={}"_format(project->BuildRoot)}),
          builder::CustomCommandLine::Make(
              {"mkdir", "-p",
               project->BuildRoot.Sub("pb", "c++").Stringify()})),
      "", true);
  makefile->AddTarget("external", {}, {}, "", true);

  std::list<std::string> clean_targets;

  std::unordered_set<std::string> recorder;
  auto gen_target = [&makefile, project, &recorder,
                     &clean_targets](rules::BuildRule *rule) {
    if (recorder.find(rule->FullQualifiedName()) != recorder.end()) {
      return;
    }
    recorder.insert(rule->FullQualifiedName());

    logger->info("Generate global target for {}, type: {}", *rule, rule->Type);

    auto working_folder = rule->WorkingFolder(project->BuildRoot);

    std::unordered_set<uint32_t> numbers;
    auto merge_numbers = [&](rules::BuildRule *rule) {
      for (auto id : rule->KeyNumbers()) {
        numbers.insert(id);
      }
    };
    std::unordered_set<std::string> _recorder;
    rule->RecursiveExecute(merge_numbers, &_recorder);
    _recorder.clear();

    auto clean_target = working_folder.Sub("clean");
    makefile->AddTarget(
        clean_target, {},
        builder::CustomCommandLines::Single(
            {"@$(MAKE)", "-f", working_folder.Sub("build.make").Stringify(),
             "clean"}),
        "", true);
    clean_targets.push_back(clean_target);

    if (rule->Type.IsCC()) {
      // cc target has build type
      for (const auto &output_format : formats) {
        std::list<std::string> deps;
        for (auto dep : rule->Dependencies) {
          deps.push_back(dep->FullQualifiedTarget(output_format.first));
        }
        deps.push_back("pre");

        makefile->AddTarget(
            rule->FullQualifiedTarget(output_format.first), deps,
            builder::CustomCommandLines::Multiple(
                builder::CustomCommandLine::Make(
                    {"@$(MAKE)", "-f",
                     working_folder.Sub("build.make").Stringify(),
                     output_format.second}),
                ::jk::rules::PrintPlain(
                    project, numbers,
                    "Built rule <cyan>{}:{}</cyan>, artifact: "
                    "[{}]",
                    rule->Package->Name, rule->Name,
                    utils::JoinString(", ",
                                      rule->ExportedFilesSimpleName(
                                          project, output_format.second),
                                      [](const auto &s) {
                                        return fmt::format("<green>{}</green>",
                                                           s);
                                      }))));
      }
    } else {
      // simple target
      std::list<std::string> deps;
      for (auto dep : rule->Dependencies) {
        if (!dep->Type.IsExternal()) {
          JK_THROW(JKBuildError(
              "ExternalProject can only depend on another ExternalProject."));
        }
        deps.push_back(dep->FullQualifiedTarget());
      }
      deps.push_back("pre");

      auto print_stmt = ::jk::rules::PrintPlain(
          project, numbers, "Built rule <cyan>{}:{}</cyan>",
          rule->Package->Name, rule->Name);

      makefile->AddTarget(
          rule->FullQualifiedTarget(), deps,
          builder::CustomCommandLines::Multiple(
              builder::CustomCommandLine::Make(
                  {"@$(MAKE)", "-f",
                   working_folder.Sub("build.make").Stringify(), "build"}),
              print_stmt),
          "", true);
      makefile->AddTarget("external", {rule->FullQualifiedTarget()});
    }
  };

  std::unordered_set<std::string> packages;
  for (auto rule : rules) {
    packages.insert("{}/BUILD"_format(rule->Package->Name));
    auto deps = rule->DependenciesInOrder();
    for (auto dep : deps) {
      packages.insert("{}/BUILD"_format(dep->Package->Name));
    }
  }

  auto regen_stmt = builder::CustomCommandLine::Make({"-@$(JK_COMMAND)"});
  std::copy(std::next(std::begin(cli::CommandLineArguments)),
            std::end(cli::CommandLineArguments),
            std::back_inserter(regen_stmt));
  auto regen_touch_stmt =
      builder::CustomCommandLine::Make({"@touch", regen_target});

  std::list<std::string> regen_deps{std::begin(packages), std::end(packages)};
  regen_deps.push_back((project->ProjectRoot.Path / "JK_ROOT").string());
  makefile->AddTarget(
      regen_target, regen_deps,
      builder::CustomCommandLines::Multiple(regen_stmt, regen_touch_stmt));

  for (auto rule : rules) {
    rule->RecursiveExecute(gen_target);

    for (const auto &output_format : formats) {
      auto tgt_name =
          fmt::format("{}/{}", rule->FullQualifiedName(), output_format.first);
      makefile->AddTarget(output_format.first, {tgt_name, "pre"}, {}, "", true);
    }
  }

  makefile->AddTarget("clean", clean_targets, {}, "", true);

  auto w = wf->Build(makefile->filename_);
  makefile->Write(w.get());
}

}  // namespace jk::core::compile

// vim: fdm=marker
