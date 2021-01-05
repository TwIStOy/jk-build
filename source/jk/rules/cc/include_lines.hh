// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/common/path.hh"

namespace jk::rules::cc {

std::list<std::string> GrepIncludeLines(const common::AbsolutePath &fp);

}  // namespace jk::rules::cc

// vim: fdm=marker
