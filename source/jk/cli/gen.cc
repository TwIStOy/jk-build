// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/gen.hh"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include "args.hxx"
#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_package_factory.hh"
#include "jk/core/models/build_rule_factory.hh"
#include "jk/core/models/dependent.hh"
#include "jk/core/models/helpers.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/actions/generate_all.hh"
#include "jk/impls/compilers/compiler_factory.hh"
#include "jk/impls/compilers/makefile/root_compiler.hh"
#include "jk/impls/writers/file_writer.hh"
#include "jk/utils/assert.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "jk/version.h"
#include "range/v3/range/conversion.hpp"
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

  core::models::Session session;

  session.Project = core::filesystem::JKProject::ResolveFrom(
      common::AbsolutePath{fs::current_path()});
  session.WriterFactory.reset(new impls::writers::FileWriterFactory());

  if (defines) {
    for (const auto &str : args::get(defines)) {
      std::vector<std::string> parts;
      utils::SplitString(str, std::back_inserter(parts), '=');
      if (parts.size() == 1) {
        session.GlobalVariables[parts[0]] = "";
      } else {
        session.GlobalVariables[parts[0]] = parts[1];
      }
    }
  }

  if (extra_flags) {
    session.ExtraFlags = args::get(extra_flags);
  }
  auto output_format = args::get(format);

  // generate global compile_commands.json
  // TODO(hawtian): fix compiledb generate
  // core::writer::JSONMergeWriterFactory json_merge_factory;

  std::vector<core::models::BuildRuleId> rules_id;
  if (args::get(old_style)) {
    session.ProjectMarker = "BLADE_ROOT";

    rules_id =
        rules_name | ranges::views::transform([](const std::string &str) {
          if (!utils::StringEndsWith(str, "BUILD")) {
            JK_THROW(
                core::JKBuildError("Only support rule file named 'BUILD'."));
          }

          auto id = core::models::ParseIdString("//{}:..."_format(str));
          utils::assertion::boolean.expect(
              id.Position == core::models::RuleRelativePosition::kAbsolute,
              "Only absolute rule is allowed in command-line.");
          assert(id.Position == core::models::RuleRelativePosition::kAbsolute);
          return id;
        }) |
        ranges::to_vector;
  } else {
    rules_id =
        rules_name | ranges::views::transform([](const std::string &str) {
          auto id = core::models::ParseIdString(str);
          utils::assertion::boolean.expect(
              id.Position == core::models::RuleRelativePosition::kAbsolute,
              "Only absolute rule is allowed in command-line.");
          return id;
        }) |
        ranges::to_vector;
  }

  core::models::BuildPackageFactory package_factory;
  core::models::BuildRuleFactory rule_factory;
  impls::compilers::CompilerFactory compiler_factory;

  // TODO(hawtian): add compilers

  auto scc = impls::actions::generate_all(
      &session, ranges::views::single(output_format), &compiler_factory,
      &package_factory, &rule_factory, rules_id);
  auto all_rules =
      core::models::IterAllRules(&package_factory) | ranges::to_vector;

  impls::compilers::makefile::RootCompiler root_compiler;

  // TODO(hawtian): global compiler for different output formats
  // generate global makefile
  root_compiler.Compile(&session, scc, all_rules,
                        rules_name | ranges::to_vector);
}

}  // namespace jk::cli

// vim: fdm=marker
