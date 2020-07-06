// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <unordered_map>

#include "jk/common/path.hh"
#include "jk/core/compile/ir.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/writer/writer.hh"

namespace jk::core::compile {

struct Compiler {
  virtual void Compile(filesystem::ProjectFileSystem *project, ir::IR *ir,
                       rules::BuildRule *rule,
                       filesystem::FileNamePatternExpander *expander =
                           &filesystem::kDefaultPatternExpander) const = 0;

  virtual ~Compiler() = default;
};

struct CompilerFactory {
  CompilerFactory();

  Compiler *FindCompiler(const std::string &name) const;

 private:
  std::unordered_map<std::string, std::unique_ptr<Compiler>> compilers_;
};

}  // namespace jk::core::compile

// vim: fdm=marker

