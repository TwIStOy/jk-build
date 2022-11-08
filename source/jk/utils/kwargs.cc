// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/kwargs.hh"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "pybind11/cast.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"

namespace jk::utils {

Kwargs::Kwargs(const pybind11::kwargs &args) {
  for (auto &it : args) {
    auto key = it.first.cast<std::string>();
    args.value_.emplace(
        key, std::make_shared<KwargsValue>(
                 pybind11::reinterpret_borrow<pybind11::object>(it.second)));
  }
}

std::string Kwargs::gen_stringify_cache() const {
  std::ostringstream oss;
  oss << "Kwargs {";
  /*
   * oss << JoinString(
   *     ", ", value_.begin(), value_.end(), [](const auto &pr) -> std::string {
   *       return fmt::format("{}: {}", pr.first,
   *                          pybind11::str(pr.second).cast<std::string>());
   *     });
   */
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
    res.push_back(std::get<0>(x.value));
  }

  return res;
}

std::vector<std::string> Kwargs::ListOptional(
    const std::string &name, std::optional<ListType> default_value) const {
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
    res.push_back(std::get<0>(x.value));
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

  return *it->second;
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
