// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/common/path.hh"

namespace jk::core::filesystem {

struct FileNamePatternExpander {
  virtual std::list<std::string> Expand(const std::string &pattern,
                                        const common::AbsolutePath &path) = 0;

  virtual ~FileNamePatternExpander() = default;
};

struct DefaultPatternExpander : public FileNamePatternExpander {
  std::list<std::string> Expand(const std::string &pattern,
                                const common::AbsolutePath &path) override;
};

extern DefaultPatternExpander kDefaultPatternExpander;

}  // namespace jk::core::filesystem

// vim: fdm=marker

