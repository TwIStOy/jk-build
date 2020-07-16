// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <unordered_map>

#include "jk/core/writer/buffer_writer.hh"
#include "jk/core/writer/writer.hh"

namespace jk::test {

struct FakeBufferWriterFactory;
struct FakeBufferWriter final : public core::writer::BufferWriter {
  FakeBufferWriter(FakeBufferWriterFactory *factory, std::string key)
      : Key(key), MyFactory(factory) {
  }

  virtual ~FakeBufferWriter();

  std::string Key;
  FakeBufferWriterFactory *MyFactory;
};

struct FakeBufferWriterFactory : public core::writer::WriterFactory {
  std::unique_ptr<core::writer::Writer> Build(const std::string &key) final;

  void DebugPrint(std::ostream &oss) const;

  std::unordered_map<std::string, std::string> Files;
};

}  // namespace jk::test

// vim: fdm=marker

