// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/interfaces/writer.hh"

namespace jk::impls::writers {

class FileWriter : public core::interfaces::Writer {
 public:
  explicit FileWriter();
  ~FileWriter();

  void open(const common::AbsolutePath &) override;

  Writer *write_line(std::string_view) override;

  Writer *write_line() override;

  Writer *write(std::string_view) override;

  void flush() override;

 protected:
  std::string path_;
  std::ostringstream oss_;
  std::ofstream ofs_;
};

class FileWriterFactory : public core::interfaces::WriterFactory {
  std::unique_ptr<core::interfaces::Writer> Create();
};

}  // namespace jk::impls::writers

// vim: fdm=marker
