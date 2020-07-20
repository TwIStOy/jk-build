// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/cli.hh"

#include "args.hxx"

namespace jk::cli {

void SubCommand::Register(args::Group &group, std::function<void()> func) {
  Cmd.reset(
      new args::Command(group, Name, Desp, [this, func](args::Subparser &p) {
        func();
        Callback(p);
      }));
}

}  // namespace jk::cli

// vim: fdm=marker
