// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/common/path.hh"

namespace jk::core::interfaces {

struct FileNamePatternExpander {
  //! Returns all filename match `pattern` in given `path`.
  virtual std::list<std::string> Expand(const std::string &pattern,
                                        const common::AbsolutePath &path) = 0;

  virtual ~FileNamePatternExpander() = default;
};

}  // namespace jk::core::interfaces
