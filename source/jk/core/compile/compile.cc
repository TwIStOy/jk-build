// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <algorithm>
#include <iterator>
#include <list>
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
#include "jk/rules/external/compiler/cmake_project.hh"
#include "jk/rules/external/compiler/shell_script.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "spdlog/spdlog.h"

namespace jk::core::compile {

using fmt::operator"" _a;
using fmt::operator"" _format;

auto logger = utils::Logger("compiler");

NopCompiler::NopCompiler(std::string name) : name_(std::move(name)) {
}

std::string NopCompiler::Name() const {
  return name_;
}

void NopCompiler::Compile(filesystem::ProjectFileSystem *,
                          writer::WriterFactory *, rules::BuildRule *,
                          filesystem::FileNamePatternExpander *) const {
}

CompilerFactory *CompilerFactory::Instance() {
  static CompilerFactory factory;
  return &factory;
}

#define NOP_COMPILER(name)                                 \
  do {                                                     \
    std::string __name = (name);                           \
    logger->debug("Register nop compiler for {}", __name); \
    compilers_[__name].reset(new NopCompiler(__name));     \
  } while (0);
#define REG_COMPILER(name, cls)                                 \
  do {                                                          \
    std::string __name = (name);                                \
    logger->debug("Register {} compiler for {}", #cls, __name); \
    compilers_[__name].reset(new cls{});                        \
  } while (0);

CompilerFactory::CompilerFactory() {
  REG_COMPILER("Makefile.cc_library",
               ::jk::rules::cc::MakefileCCLibraryCompiler);
  REG_COMPILER("Makefile.cc_binary", ::jk::rules::cc::MakefileCCBinaryCompiler);
  REG_COMPILER("Makefile.cc_test", ::jk::rules::cc::MakefileCCTestCompiler);
  REG_COMPILER("Makefile.shell_script",
               ::jk::rules::external::MakefileShellScriptCompiler);
  REG_COMPILER("Makefile.cmake_library",
               ::jk::rules::external::MakefileCMakeLibrary);
  REG_COMPILER("Makefile.proto_library",
               ::jk::rules::cc::MakefileProtoLibraryCompiler);

  REG_COMPILER("CompileDatabase.cc_library",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  REG_COMPILER("CompileDatabase.cc_binary",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  REG_COMPILER("CompileDatabase.cc_test",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  NOP_COMPILER("CompileDatabase.shell_script");
  NOP_COMPILER("CompileDatabase.cmake_library");
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
  makefile->DefaultTarget("all");

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

    logger->info("Generate global target for {}, type: {}", *rule, rule->Type);

    auto working_folder = rule->WorkingFolder(project->BuildRoot);
    if (rule->Type.IsCC()) {
      for (const auto &output_format : formats) {
        std::list<std::string> deps;
        for (auto dep : rule->Dependencies) {
          deps.push_back(dep->FullQualifiedTarget(output_format.first));
        }
        deps.push_back("pre");
        std::unordered_set<uint32_t> numbers;
        auto merge_numbers = [&](rules::BuildRule *rule) {
          for (auto id : rule->KeyNumbers()) {
            numbers.insert(id);
          }
        };
        std::unordered_set<std::string> recorder;
        rule->RecursiveExecute(merge_numbers, &recorder);
        recorder.clear();

        makefile->AddTarget(
            rule->FullQualifiedTarget(output_format.first), deps,
            builder::CustomCommandLines::Multiple(
                builder::CustomCommandLine::Make(
                    {"@$(MAKE)", "-f",
                     working_folder.Sub("build.make").Stringify(),
                     output_format.second}),
                builder::CustomCommandLine::Make(
                    {"@$(PRINT)", "--switch=$(COLOR)",
                     "--progress-num={}"_format(
                         utils::JoinString(",", numbers)),
                     "--progress-dir={}"_format(project->BuildRoot),
                     "Built target {}"_format(rule->FullQualifiedName())})));
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
              {"@$(MAKE)", "-f", working_folder.Sub("build.make").Stringify(),
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

  auto w = wf->Build(makefile->filename_);
  makefile->Write(w.get());
}

}  // namespace jk::core::compile

// vim: fdm=marker
