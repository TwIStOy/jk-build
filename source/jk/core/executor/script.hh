// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <pybind11/embed.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "jk/core/models/session.hh"
#include "jk/utils/kwargs.hh"

namespace jk::core::executor {

class __JK_HIDDEN ScriptInterpreter {
 public:
  struct EvalResult {
    std::string FuncName;
    utils::Kwargs Args;
  };

  ScriptInterpreter(models::Session *session);

  static void AddFunc(const std::string &name);

  std::vector<EvalResult> Eval(const std::string &str);
  std::vector<EvalResult> EvalFile(std::string_view filename);

 private:
  static std::unordered_set<std::string> func_names_;

  std::unique_ptr<pybind11::scoped_interpreter> interpreter_;

  std::vector<EvalResult> results_;
  pybind11::dict locals_;
};

}  // namespace jk::core::executor
