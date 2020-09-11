// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/script/script.hh"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <fstream>
#include <functional>
#include <iterator>
#include <utility>

#include "jk/common/flags.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/rules/proto_library.hh"
#include "jk/rules/external/rules/cmake_project.hh"
#include "jk/rules/external/rules/shell_script.hh"
#include "jk/utils/logging.hh"

namespace jk {
namespace core {
namespace script {

static auto logger = utils::Logger("script");

ScriptInterpreter *ScriptInterpreter::Instance() {
  static ScriptInterpreter interp;
  return &interp;
}

void ScriptInterpreter::RegHook(const std::string &name,
                                HookFunctionType func) {
  logger->debug("Registering script hook: {}", name);
  hook_functions_.push_back(HookFunction{name, std::move(func)});
}

void ScriptInterpreter::HookFunctions() {
  RegHook("cc_library", &rules::NewRuleFromScript<::jk::rules::cc::CCLibrary>);
  RegHook("cc_binary", &rules::NewRuleFromScript<::jk::rules::cc::CCBinary>);
  RegHook("cc_test", &rules::NewRuleFromScript<::jk::rules::cc::CCTest>);
  RegHook("shell_script",
          &rules::NewRuleFromScript<::jk::rules::external::ShellScript>);
  RegHook("cmake_library",
          &rules::NewRuleFromScript<::jk::rules::external::CMakeLibrary>);
  RegHook("proto_library",
          &rules::NewRuleFromScript<::jk::rules::cc::ProtoLibrary>);
}

pybind11::dict ScriptInterpreter::Initialize(rules::BuildPackage *pkg) {
  pybind11::dict locals;
  for (const auto &fun : hook_functions_) {
    auto func = fun.Function;

    locals[fun.FuncName.c_str()] =
        pybind11::cpp_function([pkg, func](const pybind11::kwargs &_args) {
          Kwargs args;
          for (const auto &it : _args) {
            auto key = it.first.cast<std::string>();
            args[key] =
                pybind11::reinterpret_borrow<pybind11::object>(it.second);
          }

          func(pkg, std::move(args));
        });
  }
  return locals;
}

void ScriptInterpreter::AddConnomLocals(pybind11::dict *locals) {
  if (common::FLAGS_platform == common::Platform::k32) {
    locals->operator[]("platform") = 32;
    locals->operator[]("PLATFORM") = 32;
  } else {
    locals->operator[]("platform") = 64;
    locals->operator[]("PLATFORM") = 64;
  }
  // TODO(hawtian): fill common
}

void ScriptInterpreter::EvalScriptContent(rules::BuildPackage *pkg,
                                          const std::string &content) {
  auto locals = Initialize(pkg);
  AddConnomLocals(&locals);

  pybind11::exec(content, pybind11::globals(), locals);
}

void ScriptInterpreter::EvalScript(rules::BuildPackage *pkg,
                                   std::string_view filename) {
  logger->debug("Eval script {}", filename);

  std::ifstream ifs(filename.data());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});

  EvalScriptContent(pkg, content);

  logger->debug("Eval script {} done.", filename);
}

ScriptInterpreter::ScriptInterpreter() {
  HookFunctions();
}

}  // namespace script
}  // namespace core
}  // namespace jk
