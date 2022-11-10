// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/gen.hh"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "args.hxx"
#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/core/executor/script.hh"
#include "jk/core/executor/worker_pool.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_package_factory.hh"
#include "jk/core/models/build_rule_factory.hh"
#include "jk/core/models/dependent.hh"
#include "jk/core/models/helpers.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/actions/generate_all.hh"
#include "jk/impls/compilers/compiledb/cc_library_compiler.hh"
#include "jk/impls/compilers/compiler_factory.hh"
#include "jk/impls/compilers/makefile/cc_binary_compiler.hh"
#include "jk/impls/compilers/makefile/cc_library_compiler.hh"
#include "jk/impls/compilers/makefile/cc_test_compiler.hh"
#include "jk/impls/compilers/makefile/proto_library_compiler.hh"
#include "jk/impls/compilers/makefile/root_compiler.hh"
#include "jk/impls/compilers/makefile/shell_script_compiler.hh"
#include "jk/impls/compilers/nop_compiler.hh"
#include "jk/impls/rules/cc_binary.hh"
#include "jk/impls/rules/cc_library.hh"
#include "jk/impls/rules/cc_test.hh"
#include "jk/impls/rules/proto_library.hh"
#include "jk/impls/rules/shell_script.hh"
#include "jk/impls/writers/file_writer.hh"
#include "jk/utils/assert.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "jk/version.h"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/any_view.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/single.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::cli {

static auto logger = utils::Logger("cli::gen");

static auto release_git_version_file_content = R"(
// Generated by JK, JK Version )" JK_VERSION
                                               R"(//

#pragma once  // NOLINT(build/header_guard)

#if !defined(BUILD_TIME)
#define BUILD_TIME __DATE__ " "  __TIME__
#endif
)";

void Generate(args::Subparser &parser) {
  args::ValueFlag<std::string> format(parser, "FORMAT",
                                      "Output format. 'makefile' or 'ninja'",
                                      {"format"}, "makefile");
  args::ValueFlag<uint32_t> platform(parser, "platform", "Only 32 or 64",
                                     {'m', "platform"}, 64);
  args::ValueFlagList<std::string> defines(
      parser, "defines", "Defines variables used in BUILD files",
      {'d', "defines"});
  args::ValueFlagList<std::string> extra_flags(
      parser, "extra_flags", "Define extra flags", {'e', "extra"});
  args::Flag old_style(parser, "old_style", "Rule pattern in old-style",
                       {"old"});

  args::PositionalList<std::string> rules_name(parser, "RULE", "Rules...");

  parser.Parse();

  auto session = std::make_unique<core::models::Session>();

  session->Project = core::filesystem::JKProject::ResolveFrom(
      common::AbsolutePath{fs::current_path()});
  session->WriterFactory.reset(new impls::writers::FileWriterFactory());
  session->Executor.reset(new core::executor::WorkerPool(10));
  session->Executor->Start();
  session->PatternExpander =
      std::make_unique<core::filesystem::DefaultPatternExpander>();
  session->CompilationDatabase = std::make_unique<core::generators::Compiledb>(
      session->Project->ProjectRoot);

  if (defines) {
    for (const auto &str : args::get(defines)) {
      std::vector<std::string> parts;
      utils::SplitString(str, std::back_inserter(parts), '=');
      if (parts.size() == 1) {
        session->GlobalVariables[parts[0]] = "";
      } else {
        session->GlobalVariables[parts[0]] = parts[1];
      }
    }
  }

  if (extra_flags) {
    session->ExtraFlags = args::get(extra_flags);
  }
  auto output_format = args::get(format);

  std::vector<core::models::BuildRuleId> rules_id;
  if (args::get(old_style)) {
    session->ProjectMarker = "BLADE_ROOT";

    for (const auto &str : rules_name) {
      if (!utils::StringEndsWith(str, "BUILD")) {
        JK_THROW(core::JKBuildError("Only support rule file named 'BUILD'."));
      }

      auto id = core::models::ParseIdString(fmt::format("//{}:...", str));
      utils::assertion::boolean.expect(
          id.Position == core::models::RuleRelativePosition::kAbsolute,
          "Only absolute rule is allowed in command-line.");
      assert(id.Position == core::models::RuleRelativePosition::kAbsolute);
      rules_id.push_back(std::move(id));
    }
  } else {
    for (const auto &str : rules_name) {
      auto id = core::models::ParseIdString(str);
      utils::assertion::boolean.expect(
          id.Position == core::models::RuleRelativePosition::kAbsolute,
          "Only absolute rule is allowed in command-line.");
      rules_id.push_back(std::move(id));
    }
  }

  auto package_factory  = std::make_unique<core::models::BuildPackageFactory>();
  auto rule_factory     = std::make_unique<core::models::BuildRuleFactory>();
  auto compiler_factory = std::make_unique<impls::compilers::CompilerFactory>();

  rule_factory->AddSimpleCreator<impls::rules::CCLibrary>("cc_library");
  rule_factory->AddSimpleCreator<impls::rules::CCBinary>("cc_binary");
  rule_factory->AddSimpleCreator<impls::rules::CCTest>("cc_test");
  rule_factory->AddSimpleCreator<impls::rules::ShellScript>("shell_script");
  rule_factory->AddSimpleCreator<impls::rules::ProtoLibrary>("proto_library");

  core::executor::ScriptInterpreter::AddFunc("cc_library");
  core::executor::ScriptInterpreter::AddFunc("cc_binary");
  core::executor::ScriptInterpreter::AddFunc("cc_test");
  core::executor::ScriptInterpreter::AddFunc("proto_library");
  core::executor::ScriptInterpreter::AddFunc("shell_script");

  compiler_factory->Register<impls::compilers::makefile::CCLibraryCompiler>(
      "makefile", "cc_library");
  compiler_factory->Register<impls::compilers::makefile::CCBinaryCompiler>(
      "makefile", "cc_binary");
  compiler_factory->Register<impls::compilers::makefile::CCTestCompiler>(
      "makefile", "cc_test");
  compiler_factory->Register<impls::compilers::makefile::ShellScriptCompiler>(
      "makefile", "shell_script");
  compiler_factory->Register<impls::compilers::makefile::ProtoLibraryCompiler>(
      "makefile", "proto_library");

  compiler_factory->Register<impls::compilers::compiledb::CCLibraryCompiler>(
      "compiledb", "cc_library");

  auto interp =
      std::make_unique<core::executor::ScriptInterpreter>(session.get());

  auto generator_names = std::vector<std::string>{output_format, "compiledb"};

  auto scc = impls::actions::generate_all(
      session.get(), interp.get(), generator_names, compiler_factory.get(),
      package_factory.get(), rule_factory.get(), rules_id);
  auto all_rules =
      core::models::IterAllRules(package_factory.get()) | ranges::to_vector;

  impls::compilers::makefile::RootCompiler root_compiler;

  auto arg_rules =
      rules_id |
      ranges::views::transform(
          [&](const core::models::BuildRuleId &id)
              -> ranges::any_view<core::models::BuildRule *> {
            auto [pkg, new_pkg] =
                package_factory->PackageUnsafe(*id.PackageName);
            if (id.RuleName == "...") {
              // all
              return pkg->IterRules();
            }
            return ranges::views::single(pkg->RulesMap[id.RuleName].get());
          }) |
      ranges::views::join | ranges::to_vector;

  root_compiler.Compile(session.get(), scc, arg_rules,
                        rules_name | ranges::to_vector);

  // waiting for all jobs finished
  session->Executor->Stop();

  assert(session->Executor->empty());

  {
    auto p = session->Project->ProjectRoot.Sub("compile_commands.json").Path;
    std::ofstream ofs(p);
    ofs << session->CompilationDatabase->dump();
    logger->info("update compiledb at {}", p.string());
  }
}

}  // namespace jk::cli

// vim: fdm=marker
