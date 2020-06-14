// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

namespace jk {
namespace core {
namespace builder {

struct Writer {
  virtual Writer &NewLine() = 0;

  virtual Writer &Comment(const std::string &comment) = 0;

  virtual Writer &Variable(const std::string &key, const std::string &value,
                           uint32_t indent = 0) = 0;

  virtual Writer &Include(const std::string &filename, bool fatal = false) = 0;

  virtual ~Writer() = default;
};

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

