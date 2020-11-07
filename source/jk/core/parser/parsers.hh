// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <regex>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parser_result.hh"
#include "jk/utils/type_traits.hh"

namespace jk::core::parser {

inline Parser<char> MakeCharAny() {
  return Parser<char>::Make([](InputStream input) {
    auto ret = ParseResult<char>(input);
    if (!input.IsEOF()) {
      auto first_char = input.RawBuffer()[0];
      ret = ParseResult<char>(input.Consume(1), first_char);
    }
    return ret;
  });
}

inline Parser<char> MakeCharEq(char ch) {
  return Parser<char>::Make([ch](InputStream input) {
    auto ret = ParseResult<char>(input);
    if (!input.IsEOF()) {
      auto first_char = input.RawBuffer()[0];
      if (ch == first_char) {
        ret = ParseResult<char>(input.Consume(1), first_char);
      }
    }
    return ret;
  });
}

template<typename F>
inline Parser<char> MakeCharPredict(F &&f) {
  return Parser<char>::Make([pred = std::forward<F>(f)](InputStream input) {
    auto ret = ParseResult<char>(input);
    if (!input.IsEOF()) {
      auto first_char = input.RawBuffer()[0];
      if (pred(first_char)) {
        ret = ParseResult<char>(input.Consume(1), first_char);
      }
    }
    return ret;
  });
}

template<typename... T, typename = std::enable_if_t<
                            (std::is_same_v<std::decay_t<T>, char> && ...)>>
inline Parser<char> MakeCharNot(T... ch) {
  return Parser<char>::Make([ch...](InputStream input) {
    auto ret = ParseResult<char>(input);
    if (!input.IsEOF()) {
      auto first_char = input.RawBuffer()[0];
      if (((ch != first_char) && ...)) {
        ret = ParseResult<char>(input.Consume(1), first_char);
      }
    }
    return ret;
  });
}

// [a, b]
inline Parser<char> MakeCharRange(char begin, char end) {
  return Parser<char>::Make([begin, end](InputStream input) {
    auto ret = ParseResult<char>(input);
    if (!input.IsEOF()) {
      auto first_char = input.RawBuffer()[0];
      if (first_char >= begin && first_char <= end) {
        ret = ParseResult<char>(input.Consume(1), first_char);
      }
    }
    return ret;
  });
}

inline Parser<std::string_view> MakeRegex(std::string_view s) {
  auto regex = std::regex(s.data(), s.size());
  return Parser<std::string_view>::Make([regex](InputStream input) {
    using result_t = ParseResult<std::string_view>;
    auto ret = result_t(input);

    auto res = std::cmatch();
    auto inputView = input.GetInput();
    if (std::regex_search(inputView.begin(), inputView.end(), res, regex,
                          std::regex_constants::match_continuous)) {
      auto matchLen = res.length(0);
      ret = result_t(input.Consume(matchLen), inputView.substr(0, matchLen));
    }

    return ret;
  });
}

inline Parser<std::string_view> MakeStringEq(std::string_view s) {
  return Parser<std::string_view>::Make([s](InputStream input) {
    using result_t = ParseResult<std::string_view>;
    auto ret = result_t(input);

    auto first_bytes = std::string_view(input.RawBuffer(), s.size());
    if (s.compare(first_bytes) == 0)
      ret = result_t(input.Consume(s.size()), first_bytes);

    return ret;
  });
}

}  // namespace jk::core::parser

// vim: fdm=marker

