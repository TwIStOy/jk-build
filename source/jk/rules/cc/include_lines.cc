// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/include_lines.hh"

#include <list>
#include <sstream>
#include <string>
#include <regex>

#include "jk/common/path.hh"

#define INCLUDE_REGEX_LINE \
  "^[ \t]*[#%][ \t]*(include|import)[ \t]*[<\"]([^\">]+)([\">])"

namespace jk::rules::cc {

std::list<std::string> GrepIncludeLines(const common::AbsolutePath &fp) {
  std::list<std::string> res;

  std::istringstream ifs(fp.Stringify());

  std::string line;
  while (std::getline(ifs, line)) {
    // TODO(hawtian):
  }

  return res;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
