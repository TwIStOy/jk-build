// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_test.hh"

#include <string>

#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/rules/cc/rules/cc_library.hh"

namespace jk::rules::cc {

std::string MakefileCCTestCompiler::Name() const {
  return "Makefile.cc_test";
}

void MakefileCCTestCompiler::AddtionalAction(
    core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder, CCLibrary *rule) const {
  // test default using 'DEBUG'
  const auto &build_type = common::FLAGS_BuildTypes[0];
  core::builder::CustomCommandLines test_statements;
  auto binary_file = working_folder.Sub(build_type).Sub(rule->ExportedFileName);
  test_statements.push_back(
      core::builder::CustomCommandLine::Make({binary_file}));
  build->AddTarget(working_folder.Sub("test"), {binary_file.Stringify()},
                   test_statements,
                   "Rule to run all test binaries in this target.");
}

}  // namespace jk::rules::cc

// vim: fdm=marker
