// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <utility>

#include "jk/core/parser/input_stream.hh"

namespace jk::core::parser {

template<typename T>
struct ParseResult {
  using result_t = T;

  template<typename I>
  explicit ParseResult(I &&i);

  template<typename I, typename O>
  ParseResult(I &&i, O &&o);

  bool Success() const {
    return static_cast<bool>(result_);
  }

  bool HasError() const {
    return !Success();
  }

  template<typename O>
  void Result(O &&o);

  const result_t &Result() const & {
    assert(Success());
    return (*result_);
  }

  result_t &&Result() && {
    assert(Success());
    return std::move(*result_);
  }

  const InputStream &GetInputStream() const & {
    return input_;
  }

  InputStream &&GetInputStream() && {
    return std::move(input_);
  }

 private:
  InputStream input_;
  std::optional<result_t> result_;
};

template<typename T>
template<typename U>
ParseResult<T>::ParseResult(U &&input) : input_(std::forward<U>(input)) {
}

template<typename T>
template<typename I, typename O>
ParseResult<T>::ParseResult(I &&i, O &&o)
    : input_(std::forward<I>(i)), result_(std::forward<O>(o)) {
}

template<typename T>
template<typename O>
void ParseResult<T>::Result(O &&o) {
  result_ = std::forward<O>(o);
}

}  // namespace jk::core::parser

// vim: fdm=marker

