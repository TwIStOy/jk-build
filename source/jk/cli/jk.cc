// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/jk.hh"

#include <iostream>

#include "args.hxx"
#include "fmt/core.h"
#include "jk/cli/cli.hh"
#include "jk/cli/download.hh"
#include "jk/cli/echo_color.hh"
#include "jk/cli/gen.hh"
#include "jk/cli/progress.hh"
#include "jk/cli/rm.hh"
#include "jk/cli/_parse.hh"
#include "jk/version.h"

namespace jk::cli {

static auto Nop = [](args::Subparser &) {
};

Cli::Cli() {
  NewSubCommand("version", "Print version message and exit.",
                [](args::Subparser &p) {
                  p.Parse();
                  std::cout << "JK version " << JK_VERSION << std::endl;
                });
  NewSubCommand("echo_color", "Print message with color.", &EchoColor);
  NewSubCommand("gen", "Generate Unix Makefile/Ninja files for given rules.",
                &Generate);
  NewSubCommand("start_progress", "Start progres...", &StartProgress);
  NewSubCommand("download", "Download file...", &DownloadFile);
  NewSubCommand("delete_file", "Delete files...", &RmFiles);
  NewSubCommand("_parse", "Internal parse file", &_Parse);
  // Add commands
}

int Cli::Run(int argc, const char *argv[]) {
  args::ArgumentParser parser("command-line Makefile/Ninja file generator");
  args::Group commands(parser, "commands");

  args::Group global_group;
  VariableGroup vg{&global_group};
  verbose_.Register(&vg, "verbose", "Verbose level.",
                    args::Matcher{"verbose", 'V'}, 0);

  args::HelpFlag help(global_group, "help", "Print this message and exit.",
                      {'h', "help"});
  args::CompletionFlag complete(parser, {"complete"});

  args::GlobalOptions globals(parser, global_group);

  for (auto &[name, cmd] : subcommands_) {
    cmd.Register(commands, [&]() {
      vg.Notify();

      if (verbose_.Value) {
        // FIXME(hawtian): fix
        // common::FLAGS_verbose = verbose_.Value.value();
      }
    });
  }

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Completion e) {
    std::cout << e.what() << std::endl;
  } catch (args::Help) {
    std::cout << parser;
  } catch (args::Error &e) {
    std::cerr << e.what() << std::endl << parser;
    return 1;
  }
  return 0;
}

void Cli::NewSubCommand(std::string Name, std::string Desp,
                        std::function<void(args::Subparser &)> Callback) {
  subcommands_[Name] = SubCommand{Name, std::move(Desp), std::move(Callback)};
}

}  // namespace jk::cli

// vim: fdm=marker
