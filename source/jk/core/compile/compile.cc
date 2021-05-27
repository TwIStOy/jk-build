// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "jk/cli/cli.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/error.h"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/rules/cc/compiler/cc_binary.hh"
#include "jk/rules/cc/compiler/cc_library.hh"
#include "jk/rules/cc/compiler/cc_test.hh"
#include "jk/rules/cc/compiler/proto_library.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/rules/common.hh"
#include "jk/rules/external/compiler/cmake_project.hh"
#include "jk/rules/external/compiler/external_library.hh"
#include "jk/rules/external/compiler/shell_script.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "spdlog/spdlog.h"

namespace jk::core::compile {

using fmt::operator"" _a;
using fmt::operator"" _format;

auto logger = utils::Logger("compiler");

NopCompiler::NopCompiler(std::string name) : name_(std::move(name)) {
}

std::string NopCompiler::Name() const {
  return name_;
}

void NopCompiler::Compile(filesystem::JKProject *, writer::WriterFactory *,
                          rules::BuildRule *,
                          filesystem::FileNamePatternExpander *) const {
}

CompilerFactory *CompilerFactory::Instance() {
  static CompilerFactory factory;
  return &factory;
}

#define NOP_COMPILER(name)                                 \
  do {                                                     \
    std::string __name = (name);                           \
    logger->debug("Register nop compiler for {}", __name); \
    compilers_[__name].reset(new NopCompiler(__name));     \
  } while (0);
#define REG_COMPILER(name, cls)                                 \
  do {                                                          \
    std::string __name = (name);                                \
    logger->debug("Register {} compiler for {}", #cls, __name); \
    compilers_[__name].reset(new cls{});                        \
  } while (0);

CompilerFactory::CompilerFactory() {
  REG_COMPILER("Makefile.cc_library",
               ::jk::rules::cc::MakefileCCLibraryCompiler);
  REG_COMPILER("Makefile.cc_binary", ::jk::rules::cc::MakefileCCBinaryCompiler);
  REG_COMPILER("Makefile.cc_test", ::jk::rules::cc::MakefileCCTestCompiler);
  REG_COMPILER("Makefile.shell_script",
               ::jk::rules::external::MakefileShellScriptCompiler);
  REG_COMPILER("Makefile.cmake_library",
               ::jk::rules::external::MakefileCMakeLibrary);
  REG_COMPILER("Makefile.proto_library",
               ::jk::rules::cc::MakefileProtoLibraryCompiler);
  REG_COMPILER("Makefile.external_library",
               ::jk::rules::external::MakefileExternalLibraryCompiler);

  REG_COMPILER("CompileDatabase.cc_library",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  REG_COMPILER("CompileDatabase.cc_binary",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  REG_COMPILER("CompileDatabase.cc_test",
               ::jk::rules::cc::CompileDatabaseCCLibraryCompiler);
  NOP_COMPILER("CompileDatabase.shell_script");
  NOP_COMPILER("CompileDatabase.cmake_library");
}

Compiler *CompilerFactory::FindCompiler(const std::string &format,
                                        const std::string &rule_type) const {
  auto name = "{}.{}"_format(format, rule_type);
  auto it = compilers_.find(name);
  if (it == compilers_.end()) {
    return nullptr;
  }
  return it->second.get();
}

}  // namespace jk::core::compile

// vim: fdm=marker
