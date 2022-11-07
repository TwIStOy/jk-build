// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/core/error.h"
#include "jk/utils/cpp_features.hh"
#include "jk/utils/str.hh"
#include "pybind11/pytypes.h"

namespace jk::utils {

class __JK_HIDDEN Kwargs final : public Stringifiable {
 public:
  using ListType   = std::vector<std::string>;
  using StringType = std::string;
  using MapType    = absl::flat_hash_map<std::string, pybind11::object>;

  Kwargs();

  StringType StringRequired(const std::string &name) const;

  ListType ListRequired(const std::string &name) const;

  StringType StringOptional(const std::string &name,
                            std::optional<StringType> default_value) const;

  ListType ListOptional(const std::string &name,
                        std::optional<ListType> default_value) const;

  bool BooleanRequired(const std::string &name) const;

  bool BooleanOptional(const std::string &name,
                       std::optional<bool> default_value) const;

  const std::string &Stringify() const final;

  template<typename T>
  auto Find(T &&key) const {
    return value_.find(std::forward<T>(key));
  }

  auto Begin() const {
    return value_.begin();
  }

  auto End() const {
    return value_.end();
  }

  decltype(auto) value() & {
    return value_;
  }

 private:
  absl::flat_hash_map<std::string, pybind11::object> value_;
};

}  // namespace jk::utils
