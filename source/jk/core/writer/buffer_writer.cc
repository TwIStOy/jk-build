// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/writer/buffer_writer.hh"

namespace jk::core::writer {

auto BufferWriter::flush() -> void {
}

std::string BufferWriter::Buffer() const {
  return oss_.str();
}

}  // namespace jk::core::writer

// vim: fdm=marker
