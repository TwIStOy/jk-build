// Copyright (c) 2020 Hawtian Wang
//

#include "test/jk/core/compile/fake_buffer_writer.hh"

namespace jk::test {

FakeBufferWriter::~FakeBufferWriter() {
  MyFactory->Files[Key] = Buffer();
}

std::unique_ptr<core::interfaces::Writer> FakeBufferWriterFactory::Create() {
  return std::unique_ptr<core::interfaces::Writer>{new FakeBufferWriter{this}};
}

void FakeBufferWriter::open(const common::AbsolutePath &k) {
  Key = k.Stringify();
}

void FakeBufferWriterFactory::DebugPrint(std::ostream &oss) const {
  std::string sep;

  oss << "FakeBufferWriterFactory Debug Print:" << std::endl;
  for (const auto &[name, content] : Files) {
    oss << sep;
    for (auto i = 0; i < name.size() + 14; i++) {
      oss << "-";
    }
    oss << std::endl;
    oss << "[[[*** File: " << name << " ***]]]" << std::endl;
    oss << content << std::endl;
  }
}

}  // namespace jk::test

// vim: fdm=marker
