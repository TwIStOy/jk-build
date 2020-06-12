// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <boost/optional.hpp>
#include <boost/optional/optional.hpp>
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
  using MapType = std::unordered_map<std::string, pybind11::object>;

  explicit Kwargs(MapType mp);

  StringType StringRequired(const std::string& name) const;

  ListType ListRequired(const std::string& name) const;

  StringType StringOptional(const std::string& name,
                            boost::optional<StringType> default_value) const;

  ListType ListOptional(const std::string& name,
                        boost::optional<ListType> default_value) const;

  bool BooleanRequired(const std::string& name) const;

  bool BooleanOptional(const std::string& name,
                       boost::optional<bool> default_value) const;

 private:
  std::unordered_map<std::string, pybind11::object> value_;
};

}  // namespace utils
}  // namespace jk

