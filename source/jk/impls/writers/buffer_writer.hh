// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <sstream>
#include <string>
#include <vector>

#include "jk/impls/writers/file_writer.hh"

namespace jk::impls::writers {

class BufferWriter : public FileWriter {
 public:
  std::string Buffer() const;

  void flush() override;
};

}  // namespace jk::impls::writers

// vim: fdm=marker
