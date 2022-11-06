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

namespace jk {
namespace utils {

Kwargs::Kwargs() {
}

const std::string &Kwargs::Stringify() const {
  if (_cached_to_string.has_func()) {
    _cached_to_string = [this] {
      std::ostringstream oss;

      oss << "Kwargs {";

      oss << JoinString(", ", value_.begin(), value_.end(),
                        [](const auto &pr) -> std::string {
                          return "{}: {}"_format(
                              pr.first,
                              pybind11::str(pr.second).cast<std::string>());
                        });

      oss << "}";

      return oss.str();
    };
  }
  return *_cached_to_string;
}

Kwargs::StringType Kwargs::StringRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!pybind11::isinstance<pybind11::str>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return it->second.cast<std::string>();
}

Kwargs::ListType Kwargs::ListRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!pybind11::isinstance<pybind11::list>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }

  return it->second.cast<ListType>();
}

Kwargs::ListType Kwargs::ListOptional(
    const std::string &name, std::optional<ListType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!pybind11::isinstance<pybind11::list>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }

  return it->second.cast<ListType>();
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

  if (!pybind11::isinstance<pybind11::str>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return it->second.cast<std::string>();
}

bool Kwargs::BooleanRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!pybind11::isinstance<pybind11::bool_>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return it->second.cast<bool>();
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

  if (!pybind11::isinstance<pybind11::bool_>(it->second)) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return it->second.cast<bool>();
}

}  // namespace utils
}  // namespace jk
