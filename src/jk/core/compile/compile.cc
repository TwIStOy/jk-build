// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <list>
#include <unordered_map>
#include <vector>

#include "fmt/core.h"
#include "jk/core/error.h"
#include "jk/core/rules/package.hh"
#include "jk/lang/cc/compiler/cc_binary.hh"
#include "jk/lang/cc/compiler/cc_library.hh"
#include "jk/lang/cc/source_file.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "spdlog/spdlog.h"

namespace jk::core::compile {

using fmt::operator"" _a;
using fmt::operator"" _format;

auto logger = utils::Logger("compiler");

CompilerFactory *CompilerFactory::Instance() {
  static CompilerFactory factory;
  return &factory;
}

CompilerFactory::CompilerFactory() {
  compilers_["Makefile.cc_library"].reset(
      new lang::cc::MakefileCCLibraryCompiler);
  compilers_["Makefile.cc_binary"].reset(
      new lang::cc::MakefileCCBinaryCompiler{});
}

Compiler *CompilerFactory::FindCompiler(const std::string &name) const {
  auto it = compilers_.find(name);
  if (it == compilers_.end()) {
    return nullptr;
  }
  return it->second.get();
}

}  // namespace jk::core::compile

// vim: fdm=marker
