// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <sstream>
#include <string>
#include <vector>

#include "jk/core/writer/writer.hh"

namespace jk::core::writer {

class BufferWriter : public Writer {
 public:
  Writer *WriteLine(const std::string &) override;

  Writer *NewLine() override;

  Writer *Write(const std::string &) override;

  Writer *Flush() override;

  std::string Buffer() const;

 private:
  std::ostringstream buffer_;
};

}  // namespace jk::core::writer

// vim: fdm=marker

