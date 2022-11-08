// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/executor/script.hh"

#include <fstream>
#include <vector>

#include "jk/core/models/session.hh"
#include "jk/utils/logging.hh"

namespace jk::core::executor {
using std::ifstream;

static auto logger = utils::Logger("script");

std::unordered_set<std::string> ScriptInterpreter::func_names_ = {};

auto ScriptInterpreter::AddFunc(const std::string &name) -> void {
  func_names_.insert(name);
}

ScriptInterpreter::ScriptInterpreter(models::Session *session) {
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

  if (session) {
    if (session->Project->Platform == filesystem::TargetPlatform::k32) {
      locals_["platform"] = 32;
      locals_["PLATFORM"] = 32;
    } else {
      locals_["platform"] = 64;
      locals_["PLATFORM"] = 64;
    }

    locals_["JK_SOURCE_DIR"] = session->Project->ProjectRoot.Stringify();
    locals_["JK_BINARY_DIR"] = session->Project->BuildRoot.Stringify();
    locals_["JK_BUNDLE_LIBRARY_PREFIX"] =
        session->Project->ExternalInstalledPrefix.Stringify();
    locals_["JK_CXX_STANDARD"] = session->Project->Config().cxx_standard;
  }

  /*
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
