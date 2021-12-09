// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/flags.hh"

#include <string>
#include <vector>

namespace jk::common {

uint32_t FLAGS_verbose = 0;

uint32_t FLAGS_terminal_columns = 0;

std::vector<std::string> FLAGS_BuildTypes = {"DEBUG", "RELEASE", "PROFILING"};

std::string FLAGS_exec_path;

}  // namespace jk::common

// vim: fdm=marker
