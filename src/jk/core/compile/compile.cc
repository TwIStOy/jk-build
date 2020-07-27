// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "fmt/core.h"
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

  makefile->DefineCommon(project);

  makefile->AddTarget("all", {"debug"}, {}, "", true);

  makefile->AddTarget(
      "pre", {},
      {fmt::format(
          "@$(JK_COMMAND) start_progress --progress-mark={} --progress-dir={}",
          project->BuildRoot.Sub("progress.mark"), project->BuildRoot)},
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
        auto build_stmt =
            fmt::format("@make -f {} {}", working_folder.Sub("build.make"),
                        output_format.second);

        std::list<std::string> deps;
        for (auto dep : rule->Dependencies) {
          deps.push_back(dep->FullQualifiedTarget(output_format.first));
        }
        deps.push_back("pre");
        makefile->AddTarget(rule->FullQualifiedTarget(output_format.first),
                            deps, {build_stmt});
      }
    } else {
      auto build_stmt = fmt::format("@make -f {} {}",
                                    working_folder.Sub("build.make"), "build");

      std::list<std::string> deps;
      for (auto dep : rule->Dependencies) {
        if (!dep->Type.IsExternal()) {
          JK_THROW(JKBuildError(
              "ExternalProject can only depend on another ExternalProject."));
        }
        deps.push_back(dep->FullQualifiedTarget());
      }
      deps.push_back("pre");
      makefile->AddTarget(rule->FullQualifiedTarget(), deps, {build_stmt}, "",
                          true);
      makefile->AddTarget("external", {rule->FullQualifiedTarget()});
    }
  };

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
