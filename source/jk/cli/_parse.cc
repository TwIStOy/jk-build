// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/cli/_parse.hh"

#include "fmt/format.h"
#include "jk/core/executor/script.hh"
#include "jk/core/filesystem/project.hh"

namespace jk::cli {

void _Parse(args::Subparser &parser) {
  args::PositionalList<std::string> rules_name(parser, "RULE", "Rules...");

  parser.Parse();

  core::executor::ScriptInterpreter::AddFunc("cc_library");
  core::executor::ScriptInterpreter::AddFunc("cc_binary");
  core::executor::ScriptInterpreter::AddFunc("cc_test");
  core::executor::ScriptInterpreter::AddFunc("shell_script");
  core::executor::ScriptInterpreter::AddFunc("proto_library");

  for (const auto &filename : rules_name) {
    auto res =
        (new core::executor::ScriptInterpreter(nullptr))->EvalFile(filename);

    fmt::print("========== {} ==========\n", filename);
    for (const auto &[tname, kwargs] : res) {
      fmt::print("fname: {}\nargs:{}\n", tname, kwargs.Stringify());
    }
  }
}

}  // namespace jk::cli
