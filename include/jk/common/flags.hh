// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>

namespace jk::common {

extern uint32_t FLAGS_verbose;

enum class Platform {
  k32,
  k64,
};

extern Platform FLAGS_platform;

}  // namespace jk::common

// vim: fdm=marker
