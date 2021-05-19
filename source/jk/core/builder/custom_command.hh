// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "jk/common/path.hh"
#include "jk/utils/cpp_features.hh"
#include "jk/utils/str.hh"

namespace jk::core::builder {

//! shell command argument with escaped
struct CustomArgument : public utils::Stringifiable {
  template<typename T,
           typename = std::enable_if_t<std::is_convertible_v<T, std::string>>>
  // NOLINTNEXTLINE(runtime/explicit)
  CustomArgument(T &&str) : Argument(std::forward<T>(str)), Raw(false) {
  }

  CustomArgument(const CustomArgument &rhs)
      : Argument(rhs.Argument), Raw(rhs.Raw) {
  }

  std::string Stringify() const final;

  std::string Argument;
  bool Raw;
};

CustomArgument operator""_c_raw(const char *s, size_t);

struct CustomCommandLine final : public std::vector<CustomArgument>,
                                 public utils::Stringifiable {
  static CustomCommandLine Make(std::initializer_list<CustomArgument> ilist);

  static CustomCommandLine FromVec(std::vector<std::string> ilist);

  std::string Stringify() const final;
};

struct CustomCommandLines final : public std::vector<CustomCommandLine>,
                                  public utils::Stringifiable {
  static CustomCommandLines Single(std::initializer_list<CustomArgument> ilist);

  template<typename... Args,
           typename = std::enable_if_t<
               (std::is_same_v<std::decay_t<Args>, CustomCommandLine> && ...)>>
  static CustomCommandLines Multiple(Args &&...args) {
    CustomCommandLines res;
    std::vector<CustomCommandLine> tmp{std::forward<Args>(args)...};
    for (auto &x : tmp) {
      res.push_back(std::move(x));
    }
    return res;
  }

  std::string Stringify() const final;
};

__JK_ALWAYS_INLINE CustomArgument operator""_c_raw(const char *s, size_t) {
  CustomArgument res(s);
  res.Raw = true;
  return res;
}

}  // namespace jk::core::builder

// vim: fdm=marker
