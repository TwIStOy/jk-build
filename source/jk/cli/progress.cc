// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/progress.hh"

#include <fstream>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::cli {

void StartProgress(args::Subparser &parser) {
  args::ValueFlag<std::string> progress_dir(
      parser, "DIR", "Progress-dir folder", {"progress-dir"},
      args::Options::Required);
  args::ValueFlag<std::string> progress_mark(
      parser, "CURRENT", "Progress-mark file", {"progress-mark"},
      args::Options::Required);

  parser.Parse();

  fs::path progress = fs::path(args::get(progress_dir)) / "Progress";
  fs::remove_all(progress);

  uint32_t count;
  {
    std::ifstream ifs(args::get(progress_mark));
    if (ifs) {
      ifs >> count;
    } else {
      JK_THROW(core::JKBuildError("Could not read from count file."));
    }
  }

  fs::create_directories(progress);
  {
    auto file = progress / "count.txt";
    std::ofstream ofs(file.string());
    if (ofs) {
      ofs << count;
    } else {
      JK_THROW(core::JKBuildError("Could not write to count file."));
    }
  }
}

}  // namespace jk::cli

// vim: fdm=marker
