// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/writer/buffer_writer.hh"

namespace jk::core::writer {

Writer *BufferWriter::WriteLine(const std::string &line) {
  buffer_ << line << std::endl;
  return this;
}

Writer *BufferWriter::NewLine() {
  buffer_ << std::endl;
  return this;
}

Writer *BufferWriter::Write(const std::string &content) {
  buffer_ << content;
  return this;
}

Writer *BufferWriter::Flush() {
  /*
   * no effect
   */
  return this;
}

Writer *BufferWriter::WriterJSON(const nlohmann::json &j) {
  buffer_ << j;
  return this;
}

std::string BufferWriter::Buffer() const {
  return buffer_.str();
}

}  // namespace jk::core::writer

// vim: fdm=marker
