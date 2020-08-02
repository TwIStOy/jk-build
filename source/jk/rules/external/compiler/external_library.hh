// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/rules/external/rules/external_library.hh"

namespace jk::rules::external {

struct MakefileExternalLibraryCompiler : public core::compile::Compiler {
  virtual std::string Name() const = 0;

  virtual void Compile(
      core::filesystem::ProjectFileSystem *project,
      core::writer::WriterFactory *wf, core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander =
          &core::filesystem::kDefaultPatternExpander) const = 0;

  struct Result {
    common::AbsolutePath OutputFolder;
    std::vector<uint32_t> Index;
    std::string DownloadTarget;
    std::string DecomressTarget;
  };

  //! return: decompressed folder
  Result DownloadAndDecompress(
      core::filesystem::ProjectFileSystem *project,
      core::output::UnixMakefile *makefile, ExternalLibrary *rule,
      const common::AbsolutePath &working_folder) const;
};

}  // namespace jk::rules::external

// vim: fdm=marker

