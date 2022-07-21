// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_test.hh"

#include <string>

#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/utils/logging.hh"

namespace jk::rules::cc {

static auto logger = utils::Logger("compiler.cc_test");

std::string MakefileCCTestCompiler::Name() const {
  return "Makefile.cc_test";
}

void MakefileCCTestCompiler::AddtionalAction(
    core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder, CCLibrary *rule) const {
  logger->debug("CCTest addtional, add test rule");

  // test default using 'DEBUG'
  const auto &build_type = common::FLAGS_BuildTypes[0];
  core::builder::CustomCommandLines test_statements;
  auto binary_file = working_folder.Sub(build_type).Sub(rule->ExportedFileName);
  test_statements.push_back(core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold", "--simple",
       fmt::format("Running test, {}", rule->FullQualifiedName())}));
  test_statements.push_back(
      core::builder::CustomCommandLine::Make({binary_file}));
  build->AddTarget("test", {binary_file.Stringify()}, test_statements,
                   "Rule to run all test binaries in this target.");
}

}  // namespace jk::rules::cc

// vim: fdm=marker
