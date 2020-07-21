// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/core/filesystem/project.hh"
#include "jk/lang/cc/rules/cc_library.hh"

namespace jk::lang::cc {

std::list<std::string> MergeDepHeaders(
    core::rules::CCLibrary *rule, core::filesystem::ProjectFileSystem *project);

}  // namespace jk::lang::cc

// vim: fdm=marker

