// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/executor/script.hh"

#include <fstream>
#include <vector>

#include "jk/utils/logging.hh"

namespace jk::core::executor {
using std::ifstream;

static auto logger = utils::Logger("script");

std::unordered_set<std::string> ScriptInterpreter::func_names_ = {};

auto ScriptInterpreter::ThreadInstance() -> ScriptInterpreter * {
  thread_local ScriptInterpreter interp;
  return &interp;
}

auto ScriptInterpreter::AddFunc(const std::string &name) -> void {
  func_names_.insert(name);
}

ScriptInterpreter::ScriptInterpreter() {
  Py_NoSiteFlag            = 1;
  Py_IgnoreEnvironmentFlag = 1;
  Py_NoUserSiteDirectory   = 1;

  interpreter_ = std::make_unique<pybind11::scoped_interpreter>();

  for (const auto &func : func_names_) {
    locals_[func.c_str()] =
        pybind11::cpp_function([this, func](const pybind11::kwargs &_args) {
          results_.push_back(EvalResult{
              .FuncName = func,
              .Args     = utils::Kwargs(_args),
          });
        });
  }

  /*
   * if (project->Platform == filesystem::TargetPlatform::k32) {
   *   (*locals)["platform"] = 32;
   *   (*locals)["PLATFORM"] = 32;
   * } else {
   *   (*locals)["platform"] = 64;
   *   (*locals)["PLATFORM"] = 64;
   * }
   *
   * if (project) {
   *   (*locals)["JK_SOURCE_DIR"] = project->ProjectRoot.Stringify();
   *   (*locals)["JK_BINARY_DIR"] = project->BuildRoot.Stringify();
   *   (*locals)["JK_BUNDLE_LIBRARY_PREFIX"] =
   *       project->ExternalInstalledPrefix.Stringify();
   *   (*locals)["JK_CXX_STANDARD"] = project->Config().cxx_standard;
   * }
   * (*locals)["e"] = pybind11::cpp_function(GetGlobalVariables);
   */
}

auto ScriptInterpreter::Eval(const std::string &str)
    -> std::vector<EvalResult> {
  results_.clear();

  try {
    pybind11::exec(str, pybind11::globals(), locals_);
  } catch (const std::runtime_error &e) {
    logger->error("Failed to execute script, what: {}", e.what());
    std::terminate();
  }

  return results_;
}

auto ScriptInterpreter::EvalFile(std::string_view filename)
    -> std::vector<EvalResult> {
  std::ifstream ifs(filename.data());
  std::string content(std::istreambuf_iterator<char>{ifs},
                      std::istreambuf_iterator<char>{});

  return Eval(content);
}

}  // namespace jk::core::executor
