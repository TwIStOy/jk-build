// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string_view>

#include "fmt/format.h"

namespace jk::core::interfaces {

struct Writer {
  virtual ~Writer() = default;

  virtual Writer *write_line(std::string_view) = 0;

  virtual Writer *write_line() = 0;

  virtual Writer *write(std::string_view) = 0;

  template<typename F, typename... Args>
  Writer *write_fmt(F &&f, Args &&...args) {
    return write(fmt::format(std::forward<F>(f), std::forward<Args>(args)...));
  }

  virtual void flush() = 0;
};

}  // namespace jk::core::interfaces
