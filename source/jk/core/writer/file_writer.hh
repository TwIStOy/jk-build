// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/writer/writer.hh"

namespace jk::core::writer {

class FileWriter : public Writer {
 public:
  explicit FileWriter(const std::string &path);

  Writer *WriteLine(const std::string &) override;

  Writer *NewLine() override;

  Writer *Write(const std::string &) override;

  Writer *WriterJSON(const nlohmann::json &j) override;

  Writer *Flush() override;

 private:
  std::string path_;
  std::ostringstream oss_;
  std::ofstream ofs_;
};

struct FileWriterFactory : public WriterFactory {
  std::unique_ptr<Writer> Build(const std::string &key) override;
};

}  // namespace jk::core::writer

// vim: fdm=marker
