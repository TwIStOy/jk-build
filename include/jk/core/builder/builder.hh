// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

namespace jk {
namespace core {
namespace builder {

class Builder {
 public:
  virtual Builder *WriteLine(const std::string &line) = 0;

  template<typename Container>
  Builder *WriteLines(Container container);
};

template<typename Container>
Builder *Builder::WriteLines(Container container) {
  for (const auto &line : container) {
    WriteLine(line);
  }

  return this;
}

}  // namespace builder
}  // namespace core
}  // namespace jk

