// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/core/error.h"
#include "jk/utils/cpp_features.hh"
#include "jk/utils/str.hh"
#include "pybind11/pytypes.h"

namespace jk::core::executor {
class ScriptInterpreter;
}  // namespace jk::core::executor

namespace jk::utils {

struct KwargsValue {
  using Ptr      = std::shared_ptr<KwargsValue>;
  using MapType  = absl::flat_hash_map<std::string, Ptr>;
  using ListType = std::vector<Ptr>;

  KwargsValue(const KwargsValue &) = default;
  KwargsValue(KwargsValue &&)      = default;

  KwargsValue(const pybind11::handle &object) {
    if (pybind11::isinstance<pybind11::list>(object)) {
      std::vector<Ptr> res;
      for (const auto &v :
           pybind11::reinterpret_borrow<pybind11::list>(object)) {
        res.emplace_back(std::make_unique<KwargsValue>(v));
      }
      value = std::move(res);
    } else if (pybind11::isinstance<pybind11::str>(object)) {
      value = object.cast<std::string>();
    } else if (pybind11::isinstance<pybind11::dict>(object)) {
      for (const auto &it :
           pybind11::reinterpret_borrow<pybind11::dict>(object)) {
        auto key = it.first.cast<std::string>();
        MapType res;
        res.emplace(key, std::make_unique<KwargsValue>(it.second));
        value = std::move(res);
      }
    }
  }

  std::variant<std::string, ListType, MapType, bool> value;
};

class Kwargs final : public Stringifiable {
 public:
  using ListType   = std::vector<KwargsValue>;
  using StringType = std::string;
  using MapType    = absl::flat_hash_map<std::string, KwargsValue>;

  Kwargs(const pybind11::kwargs &args);

  Kwargs(const Kwargs &) = default;
  Kwargs(Kwargs &&)      = default;

  StringType StringRequired(const std::string &name) const;

  std::vector<std::string> ListRequired(const std::string &name) const;

  StringType StringOptional(const std::string &name,
                            std::optional<StringType> default_value) const;

  std::vector<std::string> ListOptional(
      const std::string &name,
      std::optional<std::vector<std::string> > default_value) const;

  bool BooleanRequired(const std::string &name) const;

  bool BooleanOptional(const std::string &name,
                       std::optional<bool> default_value) const;

  std::string gen_stringify_cache() const final;

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

  friend class ::jk::core::executor::ScriptInterpreter;

 private:
  absl::flat_hash_map<std::string, KwargsValue> value_;
};

}  // namespace jk::utils
