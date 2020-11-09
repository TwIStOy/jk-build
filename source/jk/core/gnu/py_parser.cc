// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/gnu/py_parser.hh"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "jk/core/parser/combinator/convertor.hh"
#include "jk/core/parser/combinator/many.hh"
#include "jk/core/parser/combinator/or.hh"
#include "jk/core/parser/combinator/plus.hh"
#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parsers.hh"

namespace jk::core::gnu {

using parser::operator""_term;

/*
 * cc_library(name = "abc", tmp="bcd", tp=["abcd"])
 */

static auto digit = parser::MakeCharRange('0', '9');
static auto alpha = parser::MakeCharRange('a', 'z') |
                    parser::MakeCharRange('A', 'Z') | parser::MakeCharEq('_');
static auto identifier = (alpha + parser::Many(digit | alpha)) >>
                         [](const auto &tp) -> std::string {
  std::string res = "";
  res.push_back(std::get<0>(tp));
  std::copy(std::get<1>(tp).begin(), std::get<1>(tp).end(),
            std::back_inserter(res));
  return res;
};
static auto double_quote = parser::MakeCharEq('"');
static auto escape_char = (parser::MakeCharEq('\\') + parser::MakeCharAny()) >>
                          [](const auto &tp) -> char {
  auto ch = std::get<1>(tp);
  if (ch == 'n') {
    return '\n';
  } else if (ch == 'r') {
    return '\r';
  } else {
    return ch;
  }
};
static auto string_literal = (double_quote +
                              parser::Many(parser::MakeCharNot('\\', '"') |
                                           escape_char) +
                              double_quote) >>
                             [](const auto &tp) -> std::string {
  auto &vec = std::get<1>(tp);
  std::string res;
  std::copy(std::begin(vec), std::end(vec), std::back_inserter(res));
  return res;
};
static auto empty_ch = parser::MakeCharPredict([](char ch) {
  return std::isspace(ch);
});
static auto space = parser::Many(empty_ch);
static auto comma = parser::MakeCharEq(',');

static auto list_string = (parser::MakeCharEq('[') + space +
                           parser::Optional(string_literal +
                                            parser::Many(space + comma + space +
                                                         string_literal) +
                                            space + parser::Optional(comma)) +
                           space + ']'_term) >>
                          [](const auto &tp) -> std::vector<std::string> {
  std::vector<std::string> res;
  auto &elements = std::get<2>(tp);
  if (elements) {
    auto &elements_value = elements.value();
    auto &first = std::get<0>(elements_value);
    res.emplace_back(first);
    for (auto &&rhs : std::get<1>(elements_value)) {
      res.emplace_back(std::get<3>(rhs));
    }
  }
  return res;
};
static auto equal = parser::MakeCharEq('=');
static auto value = (string_literal >> [](const auto &s) -> Py::value_t {
                      return s;
                    }) |
                    ((list_string) >> [](const auto &s) -> Py::value_t {
                      return s;
                    });
static auto argument = (identifier + space + equal + space + value) >>
                       [](const auto &tp)
    -> std::pair<std::string, Py::value_t> {
  return std::make_pair(std::get<0>(tp), std::get<4>(tp));
};

static auto function_call =
    (identifier + space + parser::MakeCharEq('(') + space +
     parser::Optional(argument +
                      parser::Many(space + comma + space + argument) + space +
                      parser::Optional(comma)) +
     space + parser::MakeCharEq(')')) >>
    [](const auto &tp) -> Py::PyFunc {
  Py::PyFunc func;
  func.Name = std::get<0>(tp);
  auto &args = std::get<4>(tp);
  if (args) {
    auto &args_v = args.value();
    func.Kwargs.insert(std::get<0>(args_v));
    for (auto &arg : std::get<1>(args_v)) {
      func.Kwargs.insert(std::get<3>(arg));
    }
  }
  return func;
};

static auto functions = parser::Many(space + function_call + space) >>
                        [](const auto &tp) -> Py {
  Py res;
  for (auto &&v : tp) {
    res.Functions.push_back(std::get<1>(v));
  }
  return res;
};

#define RUN_PARSER(_parser)        \
  parser::InputStream input{text}; \
                                   \
  auto res = _parser(input);       \
  if (res.Success()) {             \
    return res.Result();           \
  }                                \
                                   \
  return {};

std::optional<Py> Py::Parse(std::string_view text) {
  RUN_PARSER(functions);
}

std::optional<std::string> Py::ParseIdentifier(std::string_view text) {
  RUN_PARSER(identifier);
}

std::optional<std::string> Py::ParseStringLiteral(std::string_view text) {
  RUN_PARSER(string_literal);
}

std::optional<std::vector<std::string>> Py::ParseStringList(
    std::string_view text) {
  RUN_PARSER(list_string);
}

std::optional<std::pair<std::string, Py::value_t>> Py::ParseArgument(
    std::string_view text) {
  RUN_PARSER(argument);
}

std::optional<Py::PyFunc> Py::ParseFunctionCall(std::string_view text) {
  RUN_PARSER(function_call);
}

}  // namespace jk::core::gnu

// vim: fdm=marker

