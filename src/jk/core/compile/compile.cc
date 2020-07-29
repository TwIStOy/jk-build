// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fmt/core.h"
#include "jk/cli/cli.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/error.h"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/external/compiler/external_project.hh"
#include "jk/lang/cc/compiler/cc_binary.hh"
#include "jk/lang/cc/compiler/cc_library.hh"
#include "jk/lang/cc/compiler/cc_test.hh"
#include "jk/lang/cc/source_file.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "spdlog/spdlog.h"

namespace jk::core::compile {

using fmt::operator"" _a;
using fmt::operator"" _format;

auto logger = utils::Logger("compiler");

CompilerFactory *CompilerFactory::Instance() {
  static CompilerFactory factory;
  return &factory;
}

CompilerFactory::CompilerFactory() {
  compilers_["Makefile.cc_library"].reset(
      new lang::cc::MakefileCCLibraryCompiler);
  compilers_["Makefile.cc_binary"].reset(
      new lang::cc::MakefileCCBinaryCompiler{});
  compilers_["Makefile.cc_test"].reset(new lang::cc::MakefileCCTestCompiler{});
  compilers_["Makefile.external_project"].reset(
      new external::MakefileExternalProjectCompiler{});

  compilers_["CompileDatabase.cc_library"].reset(
      new lang::cc::CompileDatabaseCCLibraryCompiler{});
  compilers_["CompileDatabase.cc_binary"].reset(
      new lang::cc::CompileDatabaseCCLibraryCompiler{});
  compilers_["CompileDatabase.cc_test"].reset(
      new lang::cc::CompileDatabaseCCLibraryCompiler{});
  compilers_["CompileDatabase.external_project"].reset(
      new external::CompileDatabaseExternalProjectCompiler{});
}

Compiler *CompilerFactory::FindCompiler(const std::string &format,
                                        const std::string &rule_type) const {
  auto name = "{}.{}"_format(format, rule_type);
  auto it = compilers_.find(name);
  if (it == compilers_.end()) {
    return nullptr;
  }
  return it->second.get();
}

static std::vector<std::pair<std::string, std::string>> formats = {
    {"debug", "DEBUG"}, {"release", "RELEASE"}, {"profiling", "PROFILING"}};

void MakefileGlobalCompiler::Compile(
    filesystem::ProjectFileSystem *project, writer::WriterFactory *wf,
    const std::vector<rules::BuildRule *> &rules) {
  output::UnixMakefilePtr makefile{new output::UnixMakefile(
      project->ProjectRoot.Sub("Makefile").Stringify())};

  auto regen_target = project->BuildRoot.Sub("build_files.mark").Stringify();

  makefile->DefineCommon(project);
  makefile->AddTarget("all", {"debug"}, {}, "", true);

  makefile->AddTarget(
      "pre", {regen_target},
      builder::CustomCommandLines::Single(
          {"@$(JK_COMMAND)", "start_progress",
           "--progress-mark={}"_format(project->BuildRoot.Sub("progress.mark")),
           "--progress-dir={}"_format(project->BuildRoot)}),
      "", true);
  makefile->AddTarget("external", {}, {}, "", true);

  std::unordered_set<std::string> recorder;
  auto gen_target = [&makefile, project, &recorder](rules::BuildRule *rule) {
    if (recorder.find(rule->FullQualifiedName()) != recorder.end()) {
      return;
    }
    recorder.insert(rule->FullQualifiedName());

    logger->info("Generate global target for {}, type: {}",
                 rule->FullQualifiedName(), rule->Type);

    auto working_folder = rule->WorkingFolder(project->BuildRoot);
    if (rule->Type.IsCC()) {
      for (const auto &output_format : formats) {
        std::list<std::string> deps;
        for (auto dep : rule->Dependencies) {
          deps.push_back(dep->FullQualifiedTarget(output_format.first));
        }
        deps.push_back("pre");
        makefile->AddTarget(
            rule->FullQualifiedTarget(output_format.first), deps,
            builder::CustomCommandLines::Single(
                {"@make", "-f", working_folder.Sub("build.make").Stringify(),
                 output_format.second}));
      }
    } else {
      std::list<std::string> deps;
      for (auto dep : rule->Dependencies) {
        if (!dep->Type.IsExternal()) {
          JK_THROW(JKBuildError(
              "ExternalProject can only depend on another ExternalProject."));
        }
        deps.push_back(dep->FullQualifiedTarget());
      }
      deps.push_back("pre");
      makefile->AddTarget(
          rule->FullQualifiedTarget(), deps,
          builder::CustomCommandLines::Single(
              {"@make", "-f", working_folder.Sub("build.make").Stringify(),
               "build"}),
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

  makefile->AddTarget(
      regen_target,
      std::list<std::string>{std::begin(packages), std::end(packages)},
      builder::CustomCommandLines::Multiple(regen_stmt, regen_touch_stmt));

  for (auto rule : rules) {
    rule->RecursiveExecute(gen_target);

    for (const auto &output_format : formats) {
      auto tgt_name =
          fmt::format("{}/{}", rule->FullQualifiedName(), output_format.first);
      makefile->AddTarget(output_format.first, {tgt_name, "pre"}, {}, "", true);
    }
  }

  auto w = wf->Build(makefile->filename_);
  makefile->Write(w.get());
}

}  // namespace jk::core::compile

// vim: fdm=marker
