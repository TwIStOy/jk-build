// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/kwargs.hh"

#include <concepts>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "pybind11/cast.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"
#include "range/v3/numeric/iota.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/range/primitives.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/any_view.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/iota.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/single.hpp"
#include "range/v3/view/take.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::utils {

KwargsValue::KwargsValue(const pybind11::handle &_object) {
  auto object = pybind11::reinterpret_borrow<pybind11::object>(_object);

  if (pybind11::isinstance<pybind11::list>(object)) {
    std::vector<Ptr> res;
    for (const auto &v : pybind11::reinterpret_borrow<pybind11::list>(object)) {
      res.emplace_back(std::make_shared<KwargsValue>(v));
    }
    value = std::move(res);
  } else if (pybind11::isinstance<pybind11::str>(object) ||
             pybind11::isinstance<pybind11::bytes>(object)) {
    value = object.cast<std::string>();
  } else if (pybind11::isinstance<pybind11::dict>(object)) {
    for (const auto &it :
         pybind11::reinterpret_borrow<pybind11::dict>(object)) {
      auto key = it.first.cast<std::string>();
      MapType res;
      res.emplace(key, std::make_shared<KwargsValue>(it.second));
      value = std::move(res);
    }
  } else {
    JK_THROW(core::JKBuildError("unknown type"));
  }
}

auto KwargsValue::to_string() const -> std::string {
  return std::visit(
      [](const auto &v) -> std::string {
        using T = std::remove_cvref_t<decltype(v)>;

        if constexpr (std::is_same_v<T, std::string>) {
          return fmt::format("\"{}\"", v);
        } else if constexpr (std::is_same_v<T, ListType>) {
          return "[" +
                 absl::StrJoin(v | ranges::views::transform([](auto &x) {
                                 return x->to_string();
                               }),
                               ", ") +
                 "]";
        } else if constexpr (std::is_same_v<T, MapType>) {
          return "{" +
                 absl::StrJoin(v | ranges::views::transform([](auto &x) {
                                 return x.first + ": " + x.second->to_string();
                               }),
                               ", ") +
                 "}";
        } else if constexpr (std::is_same_v<T, bool>) {
          if (v) {
            return "true";
          } else {
            return "false";
          }
        }
      },
      value);
}

Kwargs::Kwargs(const pybind11::kwargs &args) {
  for (auto &it : args) {
    auto key = it.first.cast<std::string>();
    value_.emplace(key, KwargsValue(it.second));
  }
}

std::string Kwargs::gen_stringify_cache() const {
  std::ostringstream oss;
  oss << "Kwargs {";
  oss << JoinString(
      ", ", value_.begin(), value_.end(), [](const auto &pr) -> std::string {
        return fmt::format("{}: {}", pr.first, pr.second.to_string());
      });
  oss << "}";
  return oss.str();
}

Kwargs::StringType Kwargs::StringRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 0) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return std::get<0>(it->second.value);
}

std::vector<std::string> Kwargs::ListRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 1) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }
  std::vector<std::string> res;
  for (const auto &x : std::get<1>(it->second.value)) {
    res.push_back(std::get<0>(x->value));
  }

  return res;
}

std::vector<std::string> Kwargs::ListOptional(
    const std::string &name,
    std::optional<std::vector<std::string> > default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 1) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }
  std::vector<std::string> res;
  for (const auto &x : std::get<1>(it->second.value)) {
    res.push_back(std::get<0>(x->value));
  }

  return res;
}

Kwargs::StringType Kwargs::StringOptional(
    const std::string &name, std::optional<StringType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 1) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return std::get<0>(it->second.value);
}

bool Kwargs::BooleanRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 3) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return std::get<3>(it->second.value);
}

bool Kwargs::BooleanOptional(const std::string &name,
                             std::optional<bool> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (it->second.value.index() != 3) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return std::get<3>(it->second.value);
}

}  // namespace jk::utils
