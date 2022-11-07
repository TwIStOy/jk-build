// Copyright (c) 2020 Hawtian Wang
//

#include "jk/impls/writers/buffer_writer.hh"

namespace jk::impls::writers {

auto BufferWriter::flush() -> void {
}

std::string BufferWriter::Buffer() const {
  return oss_.str();
}

}  // namespace jk::impls::writers

// vim: fdm=marker
