// Copyright (c) 2020 Hawtian Wang
//

#include <filesystem>
#include <iostream>

#include "jk/cli/cli.hh"
#include "jk/cli/jk.hh"
#include "jk/common/flags.hh"

int main(int argc, char const *argv[]) {
  std::cout << "exe path: " << std::filesystem::read_symlink("/proc/self/exe")
            << std::endl;
  FLAGS_exec_path = std::filesystem::read_symlink("/proc/self/exe");

  jk::cli::CommandLineArguments.clear();
  for (auto i = 0; i < argc; i++) {
    jk::cli::CommandLineArguments.push_back(argv[i]);
  }

  jk::cli::Cli cli;
  return cli.Run(argc, argv);
}
