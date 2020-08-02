// Copyright (c) 2020 Hawtian Wang
//

#include <iostream>

#include "jk/cli/cli.hh"
#include "jk/cli/jk.hh"

int main(int argc, char const *argv[]) {
  jk::cli::CommandLineArguments.clear();
  for (auto i = 0; i < argc; i++) {
    jk::cli::CommandLineArguments.push_back(argv[i]);
  }

  jk::cli::Cli cli;
  return cli.Run(argc, argv);
}

