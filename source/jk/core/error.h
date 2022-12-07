// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <stdexcept>
#include <string_view>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/utils/cpp_features.hh"

namespace jk::core {

class JKBuildError : public std::runtime_error {
 public:
  explicit JKBuildError(std::string_view fmt, auto &&...args)
      : std::runtime_error(fmt::format(fmt::runtime(fmt), __JK_FWD(args)...)) {
  }
};

}  // namespace jk::core
