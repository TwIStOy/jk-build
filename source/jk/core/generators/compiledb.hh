// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "jk/common/path.hh"
#include "nlohmann/json.hpp"
namespace jk::core::generators {

struct Compiledb {
 public:
  Compiledb(const common::AbsolutePath &working_folder)
      : working_folder_(working_folder.Stringify()) {
  }

  inline Compiledb &AddValues(std::vector<nlohmann::json> values) {
    std::unique_lock lk(mutex_);

    for (auto &&v : values) {
      doc_.push_back(std::move(v));
    }

    return *this;
  }

  inline auto dump() {
    return doc_.dump(2);
  }

 private:
  std::string working_folder_;

  std::mutex mutex_;
  nlohmann::json doc_;
};

}  // namespace jk::core::generators
