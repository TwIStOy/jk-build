// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/writer/writer.hh"
#include "nlohmann/json.hpp"

namespace jk::core::writer {

class JSONMergeWriter : public Writer {
 public:
  explicit JSONMergeWriter(const std::string &path);

  Writer *WriteLine(const std::string &) override;

  Writer *NewLine() override;

  Writer *Write(const std::string &) override;

  Writer *WriterJSON(const nlohmann::json &j) override;

  Writer *Flush() override;

 private:
  std::string path_;
  std::unordered_map<std::string, nlohmann::json> data_;
};

struct JSONMergeWriterFactory : public WriterFactory {
  std::unique_ptr<Writer> Build(const std::string &key) override;
};

}  // namespace jk::core::writer

// vim: fdm=marker

