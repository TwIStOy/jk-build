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

FileWriter::FileWriter() {
}

auto FileWriter::open(const common::AbsolutePath &p) -> void {
  path_ = p.Stringify();
}

auto FileWriter::write_line(std::string_view s) -> Writer * {
  ofs_ << s << "\n";
  return this;
}

auto FileWriter::write_line() -> Writer * {
  ofs_ << "\n";
  return this;
}

auto FileWriter::write(std::string_view s) -> Writer * {
  ofs_ << s;
  return this;
}

FileWriter::~FileWriter() {
  flush();
}

auto FileWriter::flush() -> void {
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
        logger->debug(
            "Because of content not change, write to file \"{}\" omitted.",
            path_);
        return;
      }
    }
  }

  logger->info("Update file {}.", path_);

  ofs_ = std::ofstream(path_);
  ofs_ << content;
  ofs_.flush();
}

}  // namespace jk::core::writer

// vim: fdm=marker
