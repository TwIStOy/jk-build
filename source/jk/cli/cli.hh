// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "args.hxx"

namespace jk::cli {

extern std::vector<std::string> CommandLineArguments;

struct SubCommand {
  std::string Name;
  std::string Desp;
  std::function<void(args::Subparser &)> Callback;

  std::unique_ptr<args::Command> Cmd;

  void Register(args::Group &group, std::function<void()>);
};

}  // namespace jk::cli

// vim: fdm=marker

