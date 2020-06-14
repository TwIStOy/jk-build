// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include "jk/core/builder/makefile_builder.hh"
#include "jk/lang/cc/cc_library.hh"

namespace jk::core::compile {

void Compiler::Compile(rules::BuildRule *rule) {
  const auto &name = rule->TypeName;
  if (name == "cc_library") {
    Compile_cc_library(rule->Downcast<rules::CCLibrary>());
  } else {
    // ...
  }
}

void Compiler::Compile_cc_library(rules::CCLibrary *rule) {
  builder::MakefileBuilder makefile;

  makefile.DefineEnvironment(const std::string &key, std::string value);
}

}  // namespace jk::core::compile

