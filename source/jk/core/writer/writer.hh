// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fmt/format.h>

#include <memory>
#include <string>

#include "nlohmann/json.hpp"

namespace jk::core::writer {

struct Writer {
  virtual Writer *WriteLine(const std::string &) = 0;

  virtual Writer *NewLine() = 0;

  virtual Writer *Write(const std::string &) = 0;

  virtual Writer *Flush() = 0;

  template<typename... Args>
  Writer *WriteLineF(const std::string &tpl, const Args &... args) {
    return WriteLine(fmt::format(tpl, args...));
  }

  virtual Writer *WriterJSON(const nlohmann::json &j) = 0;

  virtual ~Writer() = default;
};

struct WriterFactory {
  virtual std::unique_ptr<Writer> Build(const std::string &key) = 0;

  virtual ~WriterFactory() = default;
};

}  // namespace jk::core::writer

// vim: fdm=marker

