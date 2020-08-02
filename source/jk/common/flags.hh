// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <vector>

namespace jk::common {

extern uint32_t FLAGS_verbose;

enum class Platform {
  k32,
  k64,
};

extern Platform FLAGS_platform;

extern std::vector<std::string> FLAGS_BuildTypes;

std::string ToString(common::Platform plt);

extern uint32_t FLAGS_terminal_columns;

}  // namespace jk::common

// vim: fdm=marker

