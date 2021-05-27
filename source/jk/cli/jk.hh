// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

#include "args.hxx"
#include "jk/cli/cli.hh"
#include "jk/cli/variable.hh"

namespace jk::cli {

struct Cli {
  Cli();

  int Run(int argc, const char *argv[]);

  void NewSubCommand(std::string Name, std::string Desp,
                     std::function<void(args::Subparser &)> Callback);

 private:
  Variable<uint32_t> verbose_;

  std::unordered_map<std::string, SubCommand> subcommands_;
};

}  // namespace jk::cli

// vim: fdm=marker
