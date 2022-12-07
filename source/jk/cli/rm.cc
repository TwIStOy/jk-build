// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/rm.hh"

#include <iostream>
#include <string>

#include "args.hxx"
#include "jk/common/path.hh"
#include "jk/utils/str.hh"

namespace jk::cli {

void RmFiles(args::Subparser &parser) {
  args::Flag silent(parser, "silent", "silent ver", {"silent"});
  args::PositionalList<std::string> files_args(parser, "FILES",
                                               "Files to be deleted.");

  parser.Parse();

  auto files = args::get(files_args);

  for (const auto &file : files) {
    auto fp = fs::path(file);
    if (fp.is_relative()) {
      fp = fs::current_path() / fp;
    }

    if (fs::exists(fp)) {
      if (fs::is_directory(fp)) {
        fs::remove_all(fp);
        if (!args::get(silent)) {
          std::cout << fmt::format("Deleted directory {}", fp.string())
                    << std::endl;
        }
      } else {
        fs::remove(fp);
        if (!args::get(silent)) {
          std::cout << fmt::format("Deleted regular file {}", fp.string())
                    << std::endl;
        }
      }
    }
  }
}

}  // namespace jk::cli

// vim: fdm=marker
