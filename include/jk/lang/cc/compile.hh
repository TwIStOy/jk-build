// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include "jk/core/compile/compile.hh"
#include "jk/core/compile/ir.hh"
#include "jk/lang/cc/rules/cc_binary.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/lang/cc/rules/cc_test.hh"
#include "jk/lang/cc/source_file.hh"

namespace jk::lang::cc {

core::compile::ir::Statement CompileIR(
    SourceFile *source, core::filesystem::ProjectFileSystem *project,
    core::compile::ir::IR *ir);

struct CCLibraryCompiler : public core::compile::Compiler {
  void Compile(
      core::filesystem::ProjectFileSystem *project, core::compile::ir::IR *ir,
      core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;
};

//
// core::compile::ir::Statement CompileIR(
//     core::rules::CCLibrary *cc_library,
//     core::filesystem::ProjectFileSystem *project, core::compile::ir::IR *ir);
//
// core::compile::ir::Statement CompileIR(
//     core::rules::CCBinary *cc_library,
//     core::filesystem::ProjectFileSystem *project, core::compile::ir::IR *ir);
//
// core::compile::ir::Statement CompileIR(
//     core::rules::CCTest *cc_library,
//     core::filesystem::ProjectFileSystem *project, core::compile::ir::IR *ir);

}  // namespace jk::lang::cc

// vim: fdm=marker

