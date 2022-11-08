// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/core/executor/worker_pool.hh"
#include "jk/core/filesystem/configuration.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/interfaces/expander.hh"
#include "jk/core/interfaces/writer.hh"

namespace jk::core::models {

struct Session {
  uint32_t Verbose = 0;

  uint32_t TerminalColumns = 0;

  std::vector<std::string> BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

  std::string JKPath;

  std::string ProjectMarker = "JK_ROOT";

  std::unique_ptr<filesystem::JKProject> Project;

  std::unique_ptr<interfaces::FileNamePatternExpander> PatternExpander;

  std::unique_ptr<executor::WorkerPool> Executor;

  std::unique_ptr<interfaces::WriterFactory> WriterFactory;

  std::unique_ptr<filesystem::Configuration> Config;

  absl::flat_hash_map<std::string, std::string> GlobalVariables;

  std::vector<std::string> ExtraFlags;
};

}  // namespace jk::core::models
