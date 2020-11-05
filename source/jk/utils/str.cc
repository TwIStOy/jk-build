// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/str.hh"

#include <sstream>

namespace jk::utils {

std::string EscapeForShellStyle(const std::string &raw) {
  std::string result;
  for (const char *ch = raw.c_str(); *ch != '\0'; ++ch) {
    if (*ch == ' ' || *ch == '<' || *ch == '>') {
      result += '\\';
    }
    result += *ch;
  }
  return result;
}

Stringifiable::operator std::string() const {
  return Stringify();
}

}  // namespace jk::utils

// vim: fdm=marker

