// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/source_file.hh"

namespace jk::rules::cc {

extern std::vector<std::string> BuildTypes;
extern std::vector<std::string> GlobalDefines;

struct IncludesResolvingContextImpl final
    : public CCLibrary::IncludesResolvingContext {
  explicit IncludesResolvingContextImpl(core::filesystem::JKProject *p)
      : project_(p) {
  }

  core::filesystem::JKProject *Project() const override;

  core::filesystem::JKProject *project_;
};

struct MakefileCCLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
      core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;

  core::output::UnixMakefilePtr GenerateFlags(
      core::filesystem::JKProject *project, core::writer::Writer *w,
      CCLibrary *rule) const;

  core::output::UnixMakefilePtr GenerateToolchain(
      core::filesystem::JKProject *project, core::writer::Writer *w,
      CCLibrary *rule) const;

  core::output::UnixMakefilePtr GenerateBuild(
      core::filesystem::JKProject *project,
      const common::AbsolutePath &working_folder, core::writer::Writer *w,
      CCLibrary *rule,
      core::filesystem::FileNamePatternExpander *expander) const;

  //! return `ProgressNum`
  uint32_t LintSourceFile(core::filesystem::JKProject *project, CCLibrary *rule,
                          SourceFile *source_file,
                          core::output::UnixMakefile *build,
                          const common::AbsolutePath &working_folder) const;

  void MakeSourceFile(core::filesystem::JKProject *project, CCLibrary *rule,
                      const std::string &build_type, SourceFile *source_file,
                      const std::list<std::string> &headers,
                      core::output::UnixMakefile *build,
                      const common::AbsolutePath &working_folder) const;
};

struct CompileDatabaseCCLibraryCompiler : public core::compile::Compiler {
  std::string Name() const override;

  void Compile(
      core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
      core::rules::BuildRule *rule,
      core::filesystem::FileNamePatternExpander *expander) const override;
};

}  // namespace jk::rules::cc

// vim: fdm=marker
