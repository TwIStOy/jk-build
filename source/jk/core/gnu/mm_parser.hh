// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jk/utils/str.hh"

namespace jk::core::gnu {

struct MM : public utils::Stringifiable {
  std::string Target;
  std::vector<std::string> Dependencies;

  MM(std::string target, std::vector<std::string> deps)
      : Target(std::move(target)), Dependencies(std::move(deps)) {
  }

  const std::string& Stringify() const final;

  static std::optional<MM> Parse(std::string_view text);
};

}  // namespace jk::core::gnu

// vim: fdm=marker
