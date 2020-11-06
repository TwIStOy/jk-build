// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>

namespace jk::core::gnu {

struct MM {
  std::string Target;
  std::string Dependencies;

  static std::optional<MM> Parse(const std::string &text);
};

}  // namespace jk::core::gnu

// vim: fdm=marker

