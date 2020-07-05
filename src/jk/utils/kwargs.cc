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

Kwargs::Kwargs(MapType mp) : value_(std::move(mp)) {
}

std::string Kwargs::Stringify() const {
  std::ostringstream oss;

  oss << "Kwargs {";

  oss << JoinString(
      ", ", value_.begin(), value_.end(), [](const auto &pr) -> std::string {
        return "{}: {}"_format(pr.first,
                               pybind11::str(pr.second).cast<std::string>());
      });

  oss << "}";

  return oss.str();
}

// static std::vector<std::string> CastList(const pybind11::object &obj) {
//   std::vector<std::string> res;
//   std::vector<pybind11::object> lst =
//   obj.cast<std::vector<pybind11::object>>(); for (const auto &it : lst) {
//     res.push_back(it.cast<std::string>());
//   }
//   return res;
// }
//
Kwargs::StringType Kwargs::StringRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::str().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return it->second.cast<std::string>();
}

Kwargs::ListType Kwargs::ListRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::list().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }

  return it->second.cast<ListType>();
}

Kwargs::ListType Kwargs::ListOptional(
    const std::string &name, boost::optional<ListType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::list().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type list", name));
  }

  return it->second.cast<ListType>();
}

Kwargs::StringType Kwargs::StringOptional(
    const std::string &name, boost::optional<StringType> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::str().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type str", name));
  }

  return it->second.cast<std::string>();
}

bool Kwargs::BooleanRequired(const std::string &name) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::bool_().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return it->second.cast<bool>();
}

bool Kwargs::BooleanOptional(const std::string &name,
                             boost::optional<bool> default_value) const {
  auto it = value_.find(name);
  if (it == value_.end()) {
    if (default_value) {
      return default_value.value();
    }
    JK_THROW(core::JKBuildError("expect field '{}' but not found", name));
  }

  if (!it->second.get_type().is(pybind11::bool_().get_type())) {
    JK_THROW(core::JKBuildError("field '{}' expect type boolean", name));
  }

  return it->second.cast<bool>();
}

}  // namespace utils
}  // namespace jk

