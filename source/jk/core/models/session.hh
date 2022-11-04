// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

namespace jk::core::models {

struct Session {
  uint32_t Verbose = 0;

  uint32_t TerminalColumns = 0;

  std::vector<std::string> BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

  std::string CommandPath;
};

}  // namespace jk::core::models
