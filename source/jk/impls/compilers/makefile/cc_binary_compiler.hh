// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/impls/compilers/makefile/cc_library_compiler.hh"

namespace jk::impls::compilers::makefile {

struct CCBinaryCompiler : CCLibraryCompiler {
  std::string_view Name() const override;

  void generate_build_file(
      core::models::Session *session,
      const common::AbsolutePath &working_folder,
      const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
      rules::CCLibrary *rule) const override;
};

}  // namespace jk::impls::compilers::makefile
