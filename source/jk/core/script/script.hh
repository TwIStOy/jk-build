// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <pybind11/embed.h>

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/cpp_features.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace script {

class __JK_HIDDEN ScriptInterpreter {
 public:
  using Kwargs = std::unordered_map<std::string, pybind11::object>;
  using HookFunctionType =
      std::function<void(rules::BuildPackage *, const Kwargs &)>;

  static ScriptInterpreter *Instance();

  void EvalScript(filesystem::JKProject *project, rules::BuildPackage *pkg,
                  std::string_view filename);

  void EvalScriptContent(filesystem::JKProject *project,
                         rules::BuildPackage *pkg, const std::string &content);

  void RegHook(const std::string &name, HookFunctionType func);

 private:
  ScriptInterpreter();

  void HookFunctions();

  pybind11::dict Initialize(rules::BuildPackage *pkg);
  void AddConnomLocals(filesystem::JKProject *project, pybind11::dict *);

 private:
  pybind11::scoped_interpreter interpreter_;

  struct __JK_HIDDEN HookFunction {
    std::string FuncName;
    HookFunctionType Function;
  };
  std::list<HookFunction> hook_functions_;
};

}  // namespace script
}  // namespace core
}  // namespace jk
