// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/dependent.hh"

#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "jk/core/error.h"
#include "jk/core/parser/combinator/convertor.hh"
#include "jk/core/parser/combinator/many.hh"
#include "jk/core/parser/combinator/or.hh"
#include "jk/core/parser/combinator/plus.hh"
#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parsers.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"
#include "semver.hpp"

namespace jk {
namespace core {
namespace rules {

std::string BuildRuleId::Stringify() const {
  return "BuildRuleId(Id = \"{}:{}\", Position: {}, VerReq: {})"_format(
      PackageName, RuleName, Position, VersionReq);
}

static auto position_prefix =
    parser::MakeStringEq("//") | parser::MakeStringEq("##");
static auto escape_char = (parser::MakeCharEq('\\') + parser::MakeCharAny()) >>
                          [](const auto &r) -> char {
  return std::get<1>(r);
};

static auto package_name_ch = parser::MakeCharPredict([](char ch) {
                                return std::isalnum(ch) || ch == '_';
                              }) |
                              escape_char;
static auto package_name = parser::Many(package_name_ch, 1) >>
                           [](const auto &r) -> std::string {
  return utils::JoinString("", r);
};
static auto p_package =
    (package_name + parser::Many((parser::MakeCharEq('/') + package_name) >>
                                 [](const auto &r) {
                                   return std::get<1>(r);
                                 })) >>
    [](const std::tuple<std::string, std::vector<std::string>> &r)
    -> std::string {
  std::ostringstream oss;
  oss << std::get<0>(r);
  for (auto &&item : std::get<1>(r)) {
    oss << "/" << item;
  }
  auto res = oss.str();
  if (utils::StringEndsWith(res, "/BUILD")) {
    return res.substr(0, res.length() - 6);
  }
  return res;
};

static auto rule_name_ch = parser::MakeCharPredict([](char ch) {
  return std::isalnum(ch) || ch == '_' || ch == '.';
});
static auto rule_name = (parser::MakeCharEq(':') +
                         (parser::Many(rule_name_ch, 1) >>
                              [](const auto &r) -> std::string {
                           return utils::JoinString("", r);
                         })) >>
                        [](const auto &r) {
                          return std::get<1>(r);
                        };
static auto version_str = (parser::MakeCharEq('@') +
                           parser::Many(parser::MakeCharAny(), 1)) >>
                          [](const auto &r) {
                            return utils::JoinString("", std::get<1>(r));
                          };
static auto dependency_parser = parser::Optional(position_prefix) +
                                parser::Optional(p_package) + rule_name +
                                parser::Optional(version_str);

BuildRuleId ParseIdString(std::string_view str) {
  BuildRuleId res;

  if (str.empty()) {
    JK_THROW(JKBuildError("Failed to parse dependent identifier '{}'.", str));
  }

  auto pres = dependency_parser(parser::InputStream{str});
  if (!pres.Success()) {
    JK_THROW(JKBuildError("Failed to parse dep id '{}', unknown error", str));
  }

  auto [position, package, rule, ver] = pres.Result();

  if (position.has_value()) {
    if (*position == "//") {
      res.Position = RuleRelativePosition::kAbsolute;
    } else if (*position == "##") {
      res.Position = RuleRelativePosition::kBuiltin;
    }
  } else {
    if (package.has_value()) {
      res.Position = RuleRelativePosition::kRelative;
    } else {
      res.Position = RuleRelativePosition::kThis;
    }
  }

  res.PackageName = package;
  res.RuleName = rule;
  if (ver.has_value()) {
    try {
      auto v = semver::version(*ver);
      res.VersionReq = v;
    } catch (const std::exception &e) {
      JK_THROW(
          JKBuildError("Failed to parse ver req '{}', {}", *ver, e.what()));
    }
  }

  return res;
}

}  // namespace rules
}  // namespace core
}  // namespace jk
