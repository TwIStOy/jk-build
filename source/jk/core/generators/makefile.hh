// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/interfaces/writer.hh"

namespace jk::core::generators {

struct Makefile {
 public:
  explicit Makefile(common::AbsolutePath path);

  void flush(interfaces::Writer *writer);

  Makefile &Env(std::string key, std::string value);

 private:
  common::AbsolutePath path_;

  std::vector<std::pair<std::string, std::string>> environments_;
};

}  // namespace jk::core::generators
