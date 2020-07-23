// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/writer/file_writer.hh"

#include <fstream>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::core::writer {

FileWriter::FileWriter(const std::string &path) {
  fs::path p{path};
  common::AssumeFolder(p.parent_path());
  ofs_ = std::ofstream(p.string());
}

Writer *FileWriter::WriteLine(const std::string &str) {
  ofs_ << str << std::endl;
  return this;
}

Writer *FileWriter::NewLine() {
  ofs_ << std::endl;
  return this;
}

Writer *FileWriter::Write(const std::string &str) {
  ofs_ << str;
  return this;
}

Writer *FileWriter::Flush() {
  ofs_.flush();
  return this;
}

std::unique_ptr<Writer> FileWriterFactory::Build(const std::string &key) {
  return std::unique_ptr<Writer>{new FileWriter{key}};
}

}  // namespace jk::core::writer

// vim: fdm=marker

