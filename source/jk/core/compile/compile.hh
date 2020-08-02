// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/writer/writer.hh"

namespace jk::core::compile {

struct Compiler {
  //! Name of a compiler. Should in format: `{{OUTPUT_FORMAT}}.{{RULE_TYPE}}`,
  //! like 'Makefile.cc_library'
  virtual std::string Name() const = 0;

  //! Compile the `rule` in `project` and write the output into the `writer`.
  virtual void Compile(filesystem::ProjectFileSystem *project,
                       writer::WriterFactory *wf, rules::BuildRule *rule,
                       filesystem::FileNamePatternExpander *expander =
                           &filesystem::kDefaultPatternExpander) const = 0;

  virtual ~Compiler() = default;
};

struct NopCompiler final : public Compiler {
  explicit NopCompiler(std::string name);

  std::string Name() const final;

  void Compile(filesystem::ProjectFileSystem *project,
               writer::WriterFactory *wf, rules::BuildRule *rule,
               filesystem::FileNamePatternExpander *expander =
                   &filesystem::kDefaultPatternExpander) const final;

 private:
  std::string name_;
};

struct MakefileGlobalCompiler {
  void Compile(filesystem::ProjectFileSystem *project,
               writer::WriterFactory *wf,
               const std::vector<rules::BuildRule *> &rules);
};

struct CompilerFactory {
  static CompilerFactory *Instance();

  Compiler *FindCompiler(const std::string &format,
                         const std::string &rule_type) const;

 private:
  CompilerFactory();

 private:
  std::unordered_map<std::string, std::unique_ptr<Compiler>> compilers_;
};

}  // namespace jk::core::compile

// vim: fdm=marker
