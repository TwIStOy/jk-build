// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

namespace jk::common {

extern uint32_t FLAGS_verbose;

extern std::vector<std::string> FLAGS_BuildTypes;

extern uint32_t FLAGS_terminal_columns;

extern std::string FLAGS_exec_path;

}  // namespace jk::common

// vim: fdm=marker
