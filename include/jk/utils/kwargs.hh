// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jk/core/error.h"
#include "pybind11/pytypes.h"

namespace jk {
namespace utils {

class Kwargs {
 public:
  using ListType = std::vector<std::string>;
  using StringType = std::string;

  explicit Kwargs(std::unordered_map<std::string, pybind11::object>);

  StringType StringRequired(const std::string& name) const;

  ListType ListRequired(const std::string& name) const;

  // TODO(hawtian): Add optional default
  ListType ListOptional(const std::string& name) const;

 private:
  std::unordered_map<std::string, pybind11::object> value_;
};

}  // namespace utils
}  // namespace jk

