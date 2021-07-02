// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jk/core/error.h"
#include "jk/utils/cpp_features.hh"
#include "jk/utils/str.hh"
#include "pybind11/pytypes.h"

namespace jk {
namespace utils {

class __JK_HIDDEN Kwargs final : public Stringifiable {
 public:
  using ListType = std::vector<std::string>;
  using StringType = std::string;
  using MapType = std::unordered_map<std::string, pybind11::object>;

  explicit Kwargs(MapType mp);

  StringType StringRequired(const std::string &name) const;

  ListType ListRequired(const std::string &name) const;

  StringType StringOptional(const std::string &name,
                            std::optional<StringType> default_value) const;

  ListType ListOptional(const std::string &name,
                        std::optional<ListType> default_value) const;

  bool BooleanRequired(const std::string &name) const;

  bool BooleanOptional(const std::string &name,
                       std::optional<bool> default_value) const;

  std::string Stringify() const final;

  MapType::const_iterator Find(const std::string &str) const;

  MapType::const_iterator Begin() const;

  MapType::const_iterator End() const;

 private:
  std::unordered_map<std::string, pybind11::object> value_;
};

inline Kwargs::MapType::const_iterator Kwargs::Begin() const {
  return value_.begin();
}

inline Kwargs::MapType::const_iterator Kwargs::End() const {
  return value_.end();
}

}  // namespace utils
}  // namespace jk
