// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/script/script.hh"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <fstream>
#include <functional>
#include <iterator>

#include "jk/core/rules/build_rule.hh"
#include "jk/lang/cc/cc_binary.hh"
#include "jk/lang/cc/cc_library.hh"
#include "jk/lang/cc/cc_test.hh"

namespace jk {
namespace core {
namespace script {

void ScriptInterpreter::RegHook(const std::string &name,
                                HookFunctionType func) {
  hook_functions_.push_back(HookFunction{name, std::move(func)});
}

void ScriptInterpreter::HookFunctions() {
  RegHook("cc_library", &rules::NewRuleFromScript<rules::CCLibrary>);
  RegHook("cc_binary", &rules::NewRuleFromScript<rules::CCBinary>);
  RegHook("cc_test", &rules::NewRuleFromScript<rules::CCTest>);
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
  locals->operator[]("platform") = 32;
  // TODO(hawtian): fill common
}

void ScriptInterpreter::EvalScript(rules::BuildPackage *pkg,
                                   std::string_view filename) {
  auto locals = Initialize(pkg);
  AddConnomLocals(&locals);

  std::ifstream ifs(filename.data());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});

  pybind11::exec(content, pybind11::globals(), locals);
}

ScriptInterpreter::ScriptInterpreter() {
}

}  // namespace script
}  // namespace core
}  // namespace jk

