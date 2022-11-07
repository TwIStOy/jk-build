// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/cc_test_compiler.hh"

#include "range/v3/view/all.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

std::string_view CCTestCompiler::Name() const {
  return "makefile.cc_test";
}

auto CCTestCompiler::end_of_generate_build_file(
    core::generators::Makefile *makefile, core::models::Session *session,
    const common::AbsolutePath &working_folder, rules::CCLibrary *rule) const
    -> void {
  // test default using 'DEBUG'
  const auto &build_type = session->BuildTypes[0];
  core::builder::CustomCommandLines test_statements;
  auto binary_file =
      working_folder.Sub(build_type, rule->Base->Name).Stringify();
  test_statements.push_back(core::builder::CustomCommandLine::Make(
      {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold", "--simple",
       fmt::format("Running test, {}", *rule->Base->FullQualifiedName)}));
  test_statements.push_back(
      core::builder::CustomCommandLine::Make({binary_file}));

  makefile->Target("test", ranges::views::single(binary_file),
                   ranges::views::all(test_statements),
                   "Rule to run all test binaries in this target.", true);
}

}  // namespace jk::impls::compilers::makefile
