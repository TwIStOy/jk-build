// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include "absl/strings/str_join.h"
#include "jk/core/generators/makefile.hh"
#include "jk/core/models/session.hh"
#include "range/v3/range/concepts.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

core::generators::Makefile new_makefile_with_common_commands(
    core::models::Session *session, const common::AbsolutePath &working_folder);

inline auto PrintStatement(core::filesystem::JKProject *project,
                           std::string_view color, bool bold, auto numbers,
                           auto fmt_str, auto &&...args) {
  if constexpr (ranges::range<decltype(numbers)>) {
    return core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)",
         color.size() ? fmt::format("--{}", color) : "", bold ? "--bold" : "",
         fmt::format("--progress-num={}", absl::StrJoin(numbers, ",")),
         fmt::format("--progress-dir={}", project->BuildRoot.Stringify()),
         fmt::format(fmt_str, std::forward<decltype(args)>(args)...)});
  } else {
    return core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)",
         color.size() ? fmt::format("--{}", color) : "", bold ? "--bold" : "",
         fmt::format("--progress-num={}", numbers),
         fmt::format("--progress-dir={}", project->BuildRoot.Stringify()),
         fmt::format(fmt_str, std::forward<decltype(args)>(args)...)});
  }
}

}  // namespace jk::impls::compilers::makefile
