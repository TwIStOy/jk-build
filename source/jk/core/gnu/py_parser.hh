// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace jk::core::gnu {

struct Py {
  using value_t = std::variant<std::string, std::vector<std::string>>;

  struct PyFunc {
    std::string Name;
    std::unordered_map<std::string, value_t> Kwargs;
  };

  std::vector<PyFunc> Functions;

  static std::optional<Py> Parse(std::string_view text);
  static std::optional<std::string> ParseIdentifier(std::string_view text);
  static std::optional<std::string> ParseStringLiteral(std::string_view text);
  static std::optional<std::vector<std::string>> ParseStringList(
      std::string_view text);
  static std::optional<std::pair<std::string, value_t>> ParseArgument(
      std::string_view text);
  static std::optional<PyFunc> ParseFunctionCall(std::string_view text);
};

}  // namespace jk::core::gnu

// vim: fdm=marker

