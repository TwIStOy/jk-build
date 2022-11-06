// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

#include "jk/core/executor/worker_pool.hh"
#include "jk/core/filesystem/configuration.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/interfaces/writer.hh"

namespace jk::core::models {

struct Session {
  uint32_t Verbose = 0;

  uint32_t TerminalColumns = 0;

  std::vector<std::string> BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

  std::string JKPath;

  std::unique_ptr<filesystem::JKProject> Project;

  std::unique_ptr<filesystem::FileNamePatternExpander> PatternExpander;

  std::unique_ptr<executor::WorkerPool> Executor;

  std::unique_ptr<interfaces::Writer> Writer;

  std::unique_ptr<filesystem::Configuration> Config;

  std::vector<std::string> ExtraFlags;

  executor::WorkerPool Workers;
};

}  // namespace jk::core::models
