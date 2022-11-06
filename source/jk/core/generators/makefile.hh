// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/interfaces/writer.hh"
#include "range/v3/range/concepts.hpp"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/empty.hpp"

namespace jk::core::generators {

struct Makefile {
 public:
  static constexpr auto DEFAULT_TARGET = "JK_DRFAULT_TARGET";

  explicit Makefile(common::AbsolutePath path);

  void flush(interfaces::Writer *writer);

  Makefile &Env(std::string_view key, std::string_view value,
                std::string_view comment = "");

  template<ranges::range R, ranges::range U>
    requires std::convertible_to<ranges::range_value_t<U>,
                                 builder::CustomCommandLine>
  Makefile &Target(std::string_view name, R deps,
                   U cmds = ranges::views::empty<builder::CustomCommandLine>,
                   std::string_view comment = "", bool phony = false);

  Makefile &Include(std::string_view filename, std::string_view comment = "",
                    bool fatal = false);

 private:
  common::AbsolutePath path_;

  std::vector<std::pair<std::string, std::string>> environments_;
};

}  // namespace jk::core::generators
