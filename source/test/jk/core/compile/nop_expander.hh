// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"

namespace jk::test {

struct NopExpander final : core::interfaces::FileNamePatternExpander {
  std::list<std::string> Expand(const std::string &pattern,
                                const common::AbsolutePath &path) final;
};

}  // namespace jk::test

// vim: fdm=marker
