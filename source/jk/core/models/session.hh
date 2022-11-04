// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

#include "jk/core/executor/worker_pool.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"

namespace jk::core::models {

struct Session {
  uint32_t Verbose = 0;

  uint32_t TerminalColumns = 0;

  std::vector<std::string> BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

  std::string CommandPath;

  filesystem::JKProject *Project;

  filesystem::FileNamePatternExpander *PatternExpander;

  executor::WorkerPool *Executor;
};

}  // namespace jk::core::models
