// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/writer/file_writer.hh"

#include <fstream>
#include <sstream>
#include <string>

#include "boost/algorithm/string.hpp"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::core::writer {

static auto logger = utils::Logger("file_writer");

FileWriter::FileWriter(const std::string &path) : path_(path) {
  fs::path p{path};
  common::AssumeFolder(p.parent_path());
}

Writer *FileWriter::WriteLine(const std::string &str) {
  oss_ << str << std::endl;
  return this;
}

Writer *FileWriter::NewLine() {
  oss_ << std::endl;
  return this;
}

Writer *FileWriter::Write(const std::string &str) {
  oss_ << str;
  return this;
}

Writer *FileWriter::Flush() {
  auto content = oss_.str();

  {
    std::ifstream ifs(path_);
    if (ifs) {
      std::string line;
      std::ostringstream oss;
      while (std::getline(ifs, line)) {
        oss << line << std::endl;
      }

      auto old_content = boost::algorithm::trim_copy(oss.str());
      auto new_content = boost::algorithm::trim_copy(content);
      if (old_content == new_content) {
        logger->info(
            "Because of content not change, write to file \"{}\" omitted.",
            path_);
        return this;
      }
    }
  }

  logger->info("Update file {}.", path_);

  ofs_ = std::ofstream(path_);
  ofs_ << content;
  ofs_.flush();
  return this;
}

Writer *FileWriter::WriterJSON(const nlohmann::json &j) {
  oss_ << j;
  return this;
}

std::unique_ptr<Writer> FileWriterFactory::Build(const std::string &key) {
  return std::unique_ptr<Writer>{new FileWriter{key}};
}

}  // namespace jk::core::writer

// vim: fdm=marker
