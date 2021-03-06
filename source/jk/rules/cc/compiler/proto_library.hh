// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/compiler/cc_library.hh"
#include "jk/rules/cc/rules/proto_library.hh"

namespace jk::rules::cc {

struct MakefileProtoLibraryCompiler : public MakefileCCLibraryCompiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
      core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

  core::output::UnixMakefilePtr GenerateBuild(
      core::filesystem::JKProject *project,
      const common::AbsolutePath &working_folder, core::writer::Writer *w,
      ProtoLibrary *rule,
      core::filesystem::FileNamePatternExpander *expander) const;

  void MakeSourceFile(core::filesystem::JKProject *project, CCLibrary *rule,
                      const std::string &build_type, SourceFile *source_file,
                      const std::list<std::string> &headers,
                      core::output::UnixMakefile *build,
                      const common::AbsolutePath &working_folder) const;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
