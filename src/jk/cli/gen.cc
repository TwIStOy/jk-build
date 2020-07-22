// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/gen.hh"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include "args.hxx"
#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/dependent.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/file_writer.hh"
#include "jk/utils/logging.hh"

namespace jk::cli {

void Generate(args::Subparser &parser) {
  args::ValueFlag<std::string> format(parser, "FORMAT",
                                      "Output format. 'Makefile' or 'Ninja'",
                                      {"format"}, "Makefile");
  args::PositionalList<std::string> rules_name(parser, "RULE", "Rules...");
  parser.Parse();

  core::filesystem::ProjectFileSystem project{
      common::AbsolutePath{core::filesystem::ProjectRoot()},
      common::AbsolutePath{core::filesystem::BuildRoot()}};
  core::writer::FileWriterFactory writer_factory;

  std::vector<core::rules::BuildRuleId> rules_id;
  std::transform(
      std::begin(rules_name), std::end(rules_name),
      std::back_inserter(rules_id), [](const std::string &str) {
        auto id = core::rules::ParseIdString(str);
        if (id.Position != core::rules::RuleRelativePosition::kAbsolute) {
          JK_THROW(core::JKBuildError(
              "Only absolute rule is allowed in command-line."));
        }
        return id;
      });

  core::rules::BuildPackageFactory package_factory;
  std::vector<core::rules::BuildRule *> rules;
  for (const auto &id : rules_id) {
    auto pkg = package_factory.Package(id.PackageName.value());
    utils::CollisionNameStack stk;
    pkg->Initialize(&stk);

    if (id.RuleName == "...") {
      // "..." means all rules
      for (const auto &[_, rule] : pkg->Rules) {
        utils::CollisionNameStack pstk;
        utils::CollisionNameStack rstk;
        rule->BuildDependencies(&package_factory, &pstk, &rstk);
        rules.push_back(rule.get());
      }
    } else {
      auto rule = pkg->Rules[id.RuleName].get();
      if (!rule) {
        JK_THROW(core::JKBuildError("No rule named '{}' in package '{}'",
                                    id.RuleName, id.PackageName.value()));
      }
      utils::CollisionNameStack pstk;
      utils::CollisionNameStack rstk;
      rule->BuildDependencies(&package_factory, &pstk, &rstk);
      rules.push_back(rule);
    }
  }

  auto compiler_factory = core::compile::CompilerFactory::Instance();
  std::unordered_set<std::string> compiled;

  auto output_format = args::get(format);

  auto compile_rule = [&compiled, compiler_factory, &output_format, &project,
                       &writer_factory](core::rules::BuildRule *rule) {
    auto it = compiled.find(rule->FullQualifiedName());
    if (it != compiled.end()) {
      return;
    }
    compiled.insert(rule->FullQualifiedName());

    auto compiler =
        compiler_factory->FindCompiler(output_format, rule->TypeName.data());
    if (!compiler) {
      JK_THROW(core::JKBuildError("No compiler for (format: {}, TypeName: {})",
                                  output_format, rule->TypeName));
    }

    compiler->Compile(&project, &writer_factory, rule);
  };

  for (const auto &rule : rules) {
    rule->RecursiveExecute(compile_rule);
  }
}

}  // namespace jk::cli

// vim: fdm=marker

