// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>

#include "fmt/format.h"
#include "jk/utils/str.hh"
#include "semver.hpp"

namespace jk::core::models {

enum class RuleRelativePosition : uint8_t {
  kAbsolute,
  kBuiltin,
  kRelative,
  kThis,
};

//! A build-rule identifier string should in such format:
//!   rule from root:       //{PACKAGE_NAME}[/BUILD]:{RULE_NAME}
//!   rule from builtins:   ##[/BUILD]:{RULE_NAME}
//!   rule from pwd:        {PACKAGE_NAME}[/BUILD]:{RULE_NAME}
//!   rule in this package: :{RULE_NAME}
struct BuildRuleId : utils::Stringifiable {
  std::optional<std::string> PackageName;
  std::string RuleName;
  RuleRelativePosition Position;
  std::optional<semver::version> VersionReq;

  std::string gen_stringify_cache() const final;
};

BuildRuleId ParseIdString(std::string_view str);

}  // namespace jk::core::models

namespace fmt {

template<>
struct formatter<jk::core::models::RuleRelativePosition> {
  using Type = jk::core::models::RuleRelativePosition;

  template<typename ParseContext>
  constexpr auto parse(ParseContext &ctx) {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const Type &d, FormatContext &ctx) {
    switch (d) {
      case Type::kAbsolute:
        return format_to(ctx.out(), "absolute");
      case Type::kBuiltin:
        return format_to(ctx.out(), "builtin");
      case Type::kThis:
        return format_to(ctx.out(), "this");
      case Type::kRelative:
        return format_to(ctx.out(), "relative");
    }

    return format_to(ctx.out(), "unknown");
  }
};

}  // namespace fmt
