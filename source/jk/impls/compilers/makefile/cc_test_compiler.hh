// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/impls/compilers/makefile/cc_binary_compiler.hh"

namespace jk::impls::compilers::makefile {

struct CCTestCompiler : CCBinaryCompiler {
  std::string_view Name() const override;

  virtual void end_of_generate_build_file(
      core::generators::Makefile *makefile, core::models::Session *session,
      const common::AbsolutePath &working_folder, rules::CCLibrary *rule) const;
};

}  // namespace jk::impls::compilers::makefile
