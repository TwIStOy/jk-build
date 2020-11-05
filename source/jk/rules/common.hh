// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string_view>
#include <type_traits>

#include "fmt/core.h"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/utils/str.hh"

namespace jk::rules {

template<typename SeqType, typename... Args>
inline auto PrintGreen(core::filesystem::ProjectFileSystem *project,
                       const SeqType &numbers, std::string_view fmt_str,
                       const Args &... args) {
  if constexpr (std::is_integral_v<std::decay_t<SeqType>>) {
    return core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         fmt::format("--progress-num={}", numbers),
         fmt::format("--progress-dir={}", project->BuildRoot),
         fmt::format(fmt_str, args...)});
  } else {
    return core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         fmt::format("--progress-num={}", utils::JoinString(",", numbers)),
         fmt::format("--progress-dir={}", project->BuildRoot),
         fmt::format(fmt_str, args...)});
  }
}

}  // namespace jk::rules

// vim: fdm=marker

