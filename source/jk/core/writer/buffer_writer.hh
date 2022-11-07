// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <sstream>
#include <string>
#include <vector>

#include "jk/core/writer/file_writer.hh"

namespace jk::core::writer {

class BufferWriter : public FileWriter {
 public:
  std::string Buffer() const;

  void flush() override;
};

}  // namespace jk::core::writer

// vim: fdm=marker
