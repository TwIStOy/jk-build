// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace jk::core::parser {

struct ParserContext {
  std::shared_ptr<const std::string> Text;
  uint32_t CurrentIndex;

  inline char Peek() const {
    return (*Text)[CurrentIndex];
  }
};

}  // namespace jk::core::parser

// vim: fdm=marker

