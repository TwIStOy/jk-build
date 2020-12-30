// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/flags.hh"

#include <string>

namespace jk::common {

uint32_t FLAGS_verbose = 0;

uint32_t FLAGS_terminal_columns = 0;

Platform FLAGS_platform = Platform::k64;

std::vector<std::string> FLAGS_BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

std::string ToString(common::Platform plt) {
  return plt == Platform::k32 ? "32" : "64";
}

}  // namespace jk::common

// vim: fdm=marker
