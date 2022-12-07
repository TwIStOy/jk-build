// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/interfaces/writer.hh"
#include "range/v3/range/concepts.hpp"
#include "range/v3/range/primitives.hpp"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/empty.hpp"

namespace jk::core::generators {

struct Makefile {
 public:
  static constexpr auto DEFAULT_TARGET = "JK_DRFAULT_TARGET";

  explicit Makefile(common::AbsolutePath path,
                    std::vector<interfaces::WriterFactory *> writers);

  Makefile &Env(std::string_view key, std::string_view value,
                std::string_view comment = "");

  template<ranges::range R, ranges::range U>
    requires std::convertible_to<ranges::range_value_t<U>,
                                 builder::CustomCommandLine>
  Makefile &Target(std::string_view name, R &&deps,
                   U &&cmds = ranges::views::empty<builder::CustomCommandLine>,
                   std::string_view comment = "", bool phony = false) {
    print_comment(comment);
    for (auto&& dep : deps) {
      print_line(name, ": ", dep);
    }
    print_line(name, ":");
    for (const auto &stmt : cmds) {
      print_line("\t", stmt.Stringify());
    }
    if (phony) {
      print_line(".PHONY: ", name);
    }
    print_line();
    return *this;
  }

  Makefile &Include(std::string_view filename, std::string_view comment = "",
                    bool fatal = false);

  template<typename... Args>
  Makefile &Comment(Args &&...args) {
    print_comment(std::forward<Args>(args)...);
    return *this;
  }

 private:
  void print_range(auto rg) {
    for (const auto &p : rg) {
      print(p, " ");
    }
  }

  template<typename... Args>
  void print_line_impl(std::string_view first, Args &&...parts) {
    for (auto &w : writers_) {
      w->write(first);
    }
    print_line(std::forward<Args>(parts)...);
  }

  template<typename... Args>
  void print_line(Args &&...parts) {
    if constexpr (sizeof...(parts) > 1) {
      print_line_impl(std::forward<Args>(parts)...);
    } else {
      for (auto &w : writers_) {
        w->write_line(std::forward<Args>(parts)...);
      }
    }
  }

  template<typename... Args>
  void print_impl(std::string_view first, Args &&...args) {
    for (auto &w : writers_) {
      w->write(first);
    }
    print(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void print(Args &&...args) {
    if constexpr (sizeof...(args) > 1) {
      print_impl(std::forward<Args>(args)...);
    } else {
      for (auto &w : writers_) {
        w->write(std::forward<Args>(args)...);
      }
    }
  }

  void print_sep();

  template<typename... Args>
  void print_comment(Args &&...parts) {
    if ((ranges::size(parts) + ...) == 0) {
      return;
    }

    print_line("# ", std::forward<Args>(parts)...);
  }

 private:
  common::AbsolutePath path_;
  std::vector<std::unique_ptr<interfaces::Writer>> writers_;
};

}  // namespace jk::core::generators
