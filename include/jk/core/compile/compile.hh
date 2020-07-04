// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <unordered_map>

#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/cc_library.hh"

namespace jk::core::compile {

class Compiler {
 public:
  void Compile(filesystem::ProjectFileSystem *project,
               writer::WriterFactory *factory, rules::BuildRule *rule);

 private:
  void Compile_cc_library(filesystem::ProjectFileSystem *project,
                          writer::WriterFactory *factory,
                          rules::CCLibrary *rule);

 public:
  filesystem::FileNamePatternExpander *Expander =
      &filesystem::kDefaultPatternExpander;
};

}  // namespace jk::core::compile

