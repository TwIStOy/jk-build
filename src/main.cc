// Copyright (c) 2020 Hawtian Wang
//

#include <iostream>

#include "jk/cli/jk.hh"

int main(int argc, char const *argv[]) {
  jk::cli::Cli cli;
  return cli.Run(argc, argv);
}

