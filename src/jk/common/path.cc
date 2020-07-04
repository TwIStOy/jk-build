// Copyright (c) 2020 Hawtian Wang
//

#include "jk/common/path.hh"

namespace jk::common {

std::string ProjectRelativePath::Stringify() const {
  return Path.string();
}

std::string AbsolutePath::Stringify() const {
  return Path.string();
}

}  // namespace jk::common

// vim: fdm=marker

