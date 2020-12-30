// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/builder/custom_command.hh"

#include <string>
#include <vector>

#include "jk/utils/str.hh"

namespace jk::core::builder {

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

std::string CustomCommandLine::Stringify() const {
  return utils::JoinString(" ", begin(), end(), [](const CustomArgument &s) {
    if (s.Raw) {
      return s.Argument;
    } else {
      return utils::EscapeForShellStyle(s.Argument);
    }
  });
}

CustomCommandLines CustomCommandLines::Single(
    std::initializer_list<CustomArgument> ilist) {
  CustomCommandLines res;
  res.push_back(CustomCommandLine::Make(ilist));
  return res;
}

}  // namespace jk::core::builder

// vim: fdm=marker
