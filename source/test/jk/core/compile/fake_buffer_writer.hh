// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include "absl/container/flat_hash_map.h"
#include "jk/core/interfaces/writer.hh"
#include "jk/impls/writers/buffer_writer.hh"

namespace jk::test {

struct FakeBufferWriterFactory;
struct FakeBufferWriter final : public impls::writers::BufferWriter {
  FakeBufferWriter(FakeBufferWriterFactory *factory) : MyFactory(factory) {
  }

  void open(const common::AbsolutePath &) override;

  virtual ~FakeBufferWriter();

  std::string Key;
  FakeBufferWriterFactory *MyFactory;
};

struct FakeBufferWriterFactory : public core::interfaces::WriterFactory {
  std::unique_ptr<core::interfaces::Writer> Create() final;

  void DebugPrint(std::ostream &oss) const;

  absl::flat_hash_map<std::string, std::string> Files;
};

}  // namespace jk::test

// vim: fdm=marker
