// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/core/filesystem/project.hh"
#include "jk/rules/cc/rules/cc_library.hh"

namespace jk::rules::cc {

std::list<std::string> MergeDepHeaders(CCLibrary *rule,
                                       core::filesystem::JKProject *project);

}  // namespace jk::rules::cc

// vim: fdm=marker
