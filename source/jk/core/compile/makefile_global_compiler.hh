// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/core/compile/compile.hh"

namespace jk::core::compile {

struct MakefileGlobalCompiler {
  void Compile(filesystem::JKProject *project, writer::WriterFactory *wf,
               const std::vector<rules::BuildRule *> &rules);
};

}  // namespace jk::core::compile

// vim: fdm=marker
