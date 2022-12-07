// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string_view>

#include "fmt/format.h"
#include "jk/common/path.hh"

namespace jk::core::interfaces {

struct Writer {
  virtual ~Writer() = default;

  virtual void open(const common::AbsolutePath &) = 0;

  virtual Writer *write_line(std::string_view) = 0;

  virtual Writer *write_line() = 0;

  virtual Writer *write(std::string_view) = 0;

  virtual void flush() = 0;

  template<typename F, typename... Args>
  Writer *write_fmt(F &&f, Args &&...args) {
    return write(fmt::format(std::forward<F>(f), std::forward<Args>(args)...));
  }
};

struct WriterFactory {
  virtual ~WriterFactory() = default;

  virtual std::unique_ptr<Writer> Create() = 0;
};

}  // namespace jk::core::interfaces
