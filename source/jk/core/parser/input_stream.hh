// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string_view>
#include <utility>
#include <cassert>

namespace jk::core::parser {

class InputStream {
 public:
  struct Position {
    uint32_t row;
    uint32_t column;
  };

  InputStream(std::string_view s, Position p);

  explicit InputStream(std::string_view s);

  bool IsEOF() const;

  std::string_view GetInput() const;

  const char *RawBuffer() const;

  InputStream Consume(uint32_t n) const;

  uint32_t GetLineNumber() const;
  uint32_t GetColumnNumber() const;

 private:
  std::string_view str_;
  Position position_;
};

inline InputStream::InputStream(std::string_view s, Position p)
    : str_(s), position_(std::move(p)) {
}

inline InputStream::InputStream(std::string_view s) : InputStream(s, {1, 1}) {
}

inline bool InputStream::IsEOF() const {
  return str_.empty();
}

inline std::string_view InputStream::GetInput() const {
  return str_;
}

inline const char *InputStream::RawBuffer() const {
  return str_.data();
}

inline InputStream InputStream::Consume(uint32_t n) const {
  assert(n <= str_.size());

  auto new_pos = position_;

  for (auto i = 0u; i < n; ++i) {
    auto ch = str_[i];
    if (ch == '\n') {
      ++new_pos.row;
      new_pos.column = 1;
    } else {
      ++new_pos.column;
    }
  }
  return InputStream(str_.substr(n), new_pos);
}

inline uint32_t InputStream::GetLineNumber() const {
  return position_.row;
}

inline uint32_t InputStream::GetColumnNumber() const {
  return position_.column;
}

}  // namespace jk::core::parser

// vim: fdm=marker

