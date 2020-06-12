// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/kwargs.hh"

#include <sstream>

#include "jk/core/error.h"
#include "pybind11/pytypes.h"

namespace jk {
namespace utils {

Kwargs::Kwargs(MapType mp) : value_(std::move(mp)) {}

Kwargs::StringType Kwargs::StringRequired(const std::string& name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::str().get_type())) {
    throw core::JKBuildError("field '{}' expect type str", name);
  }

  return it->second.cast<std::string>();
}

Kwargs::ListType Kwargs::ListRequired(const std::string& name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::list().get_type())) {
    throw core::JKBuildError("field '{}' expect type list", name);
  }

  return it->second.cast<ListType>();
}

Kwargs::ListType Kwargs::ListOptional(
    const std::string& name, boost::optional<ListType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::list().get_type())) {
    throw core::JKBuildError("field '{}' expect type list", name);
  }

  return it->second.cast<ListType>();
}

Kwargs::StringType Kwargs::StringOptional(
    const std::string& name, boost::optional<StringType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::str().get_type())) {
    throw core::JKBuildError("field '{}' expect type str", name);
  }

  return it->second.cast<std::string>();
}

bool Kwargs::BooleanRequired(const std::string& name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::bool_().get_type())) {
    throw core::JKBuildError("field '{}' expect type boolean", name);
  }

  return it->second.cast<bool>();
}

bool Kwargs::BooleanOptional(const std::string& name,
                             boost::optional<bool> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    throw core::JKBuildError("expect field '{}' but not found", name);
  }

  if (!it->second.get_type().is(pybind11::bool_().get_type())) {
    throw core::JKBuildError("field '{}' expect type boolean", name);
  }

  return it->second.cast<bool>();
}

}  // namespace utils
}  // namespace jk

