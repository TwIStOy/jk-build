// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/mm_parser.hh"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "boost/algorithm/string.hpp"
#include "fmt/core.h"
#include "jk/core/parser/combinator/convertor.hh"
#include "jk/core/parser/combinator/many.hh"
#include "jk/core/parser/combinator/or.hh"
#include "jk/core/parser/combinator/plus.hh"
#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parsers.hh"
#include "jk/utils/str.hh"

namespace jk::core::gnu {

std::string MM::Stringify() const {
  return fmt::format("MM(target: '{}', deps: {})", Target,
                     utils::JoinString(", ", Dependencies));
}

static auto not_empty = parser::MakeCharPredict([](char ch) {
  return !std::isspace(ch);
});

static auto empty_ch = parser::MakeCharPredict([](char ch) {
  return std::isspace(ch);
});

static auto espaced_char = (parser::MakeCharEq('\\') + parser::MakeCharAny()) >>
                           [](std::tuple<char, char> r) -> char {
  return std::get<1>(r);
};

static auto target_char = parser::MakeCharPredict([](char ch) {
  if (std::isspace(ch)) {
    return false;
  }
  return ch != '\\' && ch != ':';
});

static auto rule_ch = target_char | espaced_char;

static auto target_name =
    (parser::Many(empty_ch) + parser::Many(rule_ch, 1) +
     parser::Many(empty_ch)) >>
    [](const std::tuple<std::vector<char>, std::vector<char>, std::vector<char>>
           &rp) {
      auto ch = std::get<1>(rp);
      std::string res;
      std::copy(std::begin(ch), std::end(ch), std::back_inserter(res));
      return res;
    };

static auto mm =
    (target_name + parser::MakeCharEq(':') + parser::Many(target_name, 0)) >>
    [](const std::tuple<std::string, char, std::vector<std::string>> &tp) {
      return MM{std::get<0>(tp), std::get<2>(tp)};
    };

std::optional<MM> MM::Parse(std::string_view text) {
  auto res = mm(parser::InputStream{text});
  if (res.Success()) {
    return res.Result();
  }

  return {};
}

}  // namespace jk::core::gnu

// vim: fdm=marker
