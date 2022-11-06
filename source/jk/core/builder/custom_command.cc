// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/builder/custom_command.hh"

#include <string>
#include <vector>

#include "absl/strings/str_join.h"
#include "fmt/format.h"
#include "jk/utils/str.hh"

namespace jk::core::builder {

const std::string &CustomArgument::Stringify() const {
  return Argument;
}

CustomCommandLine CustomCommandLine::Make(
    std::initializer_list<CustomArgument> ilist) {
  CustomCommandLine res;
  for (auto &i : ilist) {
    res.push_back(std::move(i));
  }
  return res;
}

CustomCommandLine CustomCommandLine::FromVec(std::vector<std::string> ilist) {
  CustomCommandLine res;
  for (auto &i : ilist) {
    res.push_back(std::move(i));
  }
  return res;
}

const std::string &CustomCommandLine::Stringify() const {
  if (!_cached_to_string.has_func()) {
    _cached_to_string = [this] {
      return utils::JoinString(
          " ", begin(), end(), [](const CustomArgument &s) {
            if (s.Raw) {
              return s.Argument;
            } else {
              return utils::EscapeForShellStyle(s.Argument);
            }
          });
    };
  }

  return *_cached_to_string;
}

CustomCommandLines CustomCommandLines::Single(
    std::initializer_list<CustomArgument> ilist) {
  CustomCommandLines res;
  res.push_back(CustomCommandLine::Make(std::move(ilist)));
  return res;
}

const std::string &CustomCommandLines::Stringify() const {
  if (!_cached_to_string.has_func()) {
    _cached_to_string = [this] {
      return utils::JoinString("\n", begin(), end(), [](const auto &s) {
        return s.Stringify();
      });
    };
  }
  return *_cached_to_string;
}

}  // namespace jk::core::builder

// vim: fdm=marker
