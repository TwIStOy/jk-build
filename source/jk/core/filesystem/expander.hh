// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/interfaces/expander.hh"

namespace jk::core::filesystem {

struct DefaultPatternExpander : public interfaces::FileNamePatternExpander {
  std::list<std::string> Expand(const std::string &pattern,
                                const common::AbsolutePath &path) override;
};

extern DefaultPatternExpander kDefaultPatternExpander;

}  // namespace jk::core::filesystem

// vim: fdm=marker
