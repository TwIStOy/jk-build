// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/script/script.hh"

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>

#include <codecvt>
#include <exception>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "jk/common/flags.hh"
#include "jk/core/filesystem/project.hh"
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

std::unordered_map<std::string, std::string> GlobalVariables;

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

std::optional<std::string> GetGlobalVariables(const std::string &str) {
  if (auto it = GlobalVariables.find(str); it == GlobalVariables.end()) {
    return {};
  } else {
    return it->second;
  }
}

void ScriptInterpreter::AddConnomLocals(filesystem::JKProject *project,
                                        pybind11::dict *locals) {
  if (project->Platform == filesystem::TargetPlatform::k32) {
    (*locals)["platform"] = 32;
    (*locals)["PLATFORM"] = 32;
  } else {
    (*locals)["platform"] = 64;
    (*locals)["PLATFORM"] = 64;
  }

  if (project) {
    (*locals)["JK_SOURCE_DIR"] = project->ProjectRoot.Stringify();
    (*locals)["JK_BINARY_DIR"] = project->BuildRoot.Stringify();
    (*locals)["JK_BUNDLE_LIBRARY_PREFIX"] =
        project->ExternalInstalledPrefix.Stringify();
    (*locals)["JK_CXX_STANDARD"] = project->Config().cxx_standard;
  }
  (*locals)["e"] = pybind11::cpp_function(GetGlobalVariables);
  // TODO(hawtian): fill common
}

void ScriptInterpreter::EvalScriptContent(filesystem::JKProject *project,
                                          rules::BuildPackage *pkg,
                                          const std::string &content) {
  auto locals = Initialize(pkg);
  AddConnomLocals(project, &locals);

  try {
    pybind11::exec(content, pybind11::globals(), locals);
  } catch (const std::runtime_error &e) {
    logger->error("Failed to execute script in {}, what: {}", pkg->Path,
                  e.what());
    std::terminate();
  }
}

void ScriptInterpreter::EvalScript(filesystem::JKProject *project,
                                   rules::BuildPackage *pkg,
                                   std::string_view filename) {
  logger->debug("Eval script {}", filename);

  std::ifstream ifs(filename.data());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});

  EvalScriptContent(project, pkg, content);

  logger->debug("Eval script {} done.", filename);
}

ScriptInterpreter::ScriptInterpreter() {
  HookFunctions();

  Py_NoSiteFlag = 1;
  Py_IgnoreEnvironmentFlag = 1;
  Py_NoUserSiteDirectory = 1;

  interpreter_ = std::make_unique<pybind11::scoped_interpreter>();
}

}  // namespace script
}  // namespace core
}  // namespace jk
