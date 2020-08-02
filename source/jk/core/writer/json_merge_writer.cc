// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/writer/json_merge_writer.hh"

#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jk/utils/logging.hh"

namespace jk::core::writer {

struct CompileCommand {
  std::string file;
  std::string directory;
  std::vector<std::string> arguments;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompileCommand, file, directory, arguments);

JSONMergeWriter::JSONMergeWriter(const std::string &path) : path_(path) {
  try {
    std::ifstream ifs(path);
    nlohmann::json j;
    ifs >> j;

    for (const auto &item : j) {
      CompileCommand cmd = item;

      data_[cmd.file] = std::move(cmd);
    }
  } catch (...) {
    utils::Logger("writer")->warn(
        "Read and parse compile_commands.json failed. Old file will be "
        "ignored.");
    data_.clear();
  }
}

Writer *JSONMergeWriter::WriteLine(const std::string &) {
  return this;
}

Writer *JSONMergeWriter::NewLine() {
  return this;
}

Writer *JSONMergeWriter::Write(const std::string &) {
  return this;
}

Writer *JSONMergeWriter::WriterJSON(const nlohmann::json &j) {
  CompileCommand cmd = j;
  data_[cmd.file] = cmd;
  return this;
}

Writer *JSONMergeWriter::Flush() {
  std::vector<CompileCommand> commands;

  for (const auto &[_, cmd] : data_) {
    commands.push_back(cmd);
  }

  nlohmann::json j = commands;
  std::ofstream ofs(path_);
  ofs << j.dump(2) << std::endl;

  return this;
}

std::unique_ptr<Writer> JSONMergeWriterFactory::Build(const std::string &key) {
  return std::unique_ptr<Writer>{new JSONMergeWriter{key}};
}

}  // namespace jk::core::writer

// vim: fdm=marker

