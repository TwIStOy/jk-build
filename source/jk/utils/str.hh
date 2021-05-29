// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "boost/optional.hpp"
#include "jk/utils/cpp_features.hh"
#include "semver.hpp"

namespace jk {

using fmt::operator"" _format;

namespace utils {

struct Stringifiable {
  //! Returns the result of stringify current object
  virtual std::string Stringify() const = 0;

  operator std::string() const;

  virtual ~Stringifiable() = default;
};

__JK_ALWAYS_INLINE Stringifiable::operator std::string() const {
  return Stringify();
}

/**
 * Returns the result of stringify object `v`
 */
template<typename T, typename = std::enable_if<
                         std::is_base_of_v<Stringifiable, std::decay_t<T>>>>
__JK_ALWAYS_INLINE std::string ToString(const T &v) {
  return v.Stringify();
}

/**
 * Returns the result of stringify object `v`
 */
template<typename T, typename = std::enable_if<
                         std::is_base_of_v<Stringifiable, std::decay_t<T>>>>
__JK_ALWAYS_INLINE std::string ToString(T *v) {
  return v->Stringify();
}

__JK_ALWAYS_INLINE std::ostream &operator<<(std::ostream &oss,
                                            const Stringifiable &rhs) {
  return oss << rhs.Stringify();
}

/**
 * Returns if `full_string` starts with `prefix`
 */
bool StringStartsWith(std::string_view full_string, std::string_view prefix);

/**
 * Returns if `full_string` ends with `suffix`
 */
bool StringEndsWith(std::string_view full_string, std::string_view suffix);

template<typename InputIterator>
typename InputIterator::value_type __DefaultUnary(
    const typename InputIterator::value_type &v) {
  return v;
}

/**
 * Returns a string which is the concatenation of the objects' stringified
 * result in the range of [begin, end). The seperator between elements is
 * `seperator`.
 */
template<
    typename InputIterator,
    typename UnaryOperator = decltype(__DefaultUnary<InputIterator>),
    typename = typename std::iterator_traits<InputIterator>::iterator_category>
std::string JoinString(std::string_view separator, InputIterator begin,
                       InputIterator end,
                       UnaryOperator func = &__DefaultUnary<InputIterator>) {
  std::ostringstream oss;
  bool first = true;
  for (auto it = begin; it != end; ++it) {
    if (first) {
      first = false;
    } else {
      oss << separator;
    }
    oss << func(*it);
  }
  return oss.str();
}

/**
 * Returns a string which is the concatenation of the objects' stringified
 * result in the iterable. The seperator between elements is `seperator`.
 */
template<typename Container,
         typename UnaryOperator =
             decltype(__DefaultUnary<typename Container::const_iterator>),
         typename = decltype(std::declval<Container>().begin(),
                             std::declval<Container>().end())>
__JK_ALWAYS_INLINE std::string JoinString(
    std::string_view separator, Container container,
    UnaryOperator func = &__DefaultUnary<typename Container::const_iterator>) {
  return JoinString(std::move(separator), std::begin(container),
                    std::end(container), func);
}

/**
 * Returns a string which replace all char `from` to the reuslt of stringified
 * object `to` in given string `old`.
 */
template<typename T>
inline std::string Replace(std::string_view old, char from, const T &to) {
  std::ostringstream oss;

  for (auto x : old) {
    if (x == from) {
      oss << to;
    } else {
      oss << x;
    }
  }

  return oss.str();
}

/**
 * Replace all occurrences of substring pattern to new_str. If optional argument
 * count is given, only the first count occurrences are replaced.
 */
void ReplaceAllSlow(std::string *text, std::string_view pattern,
                    std::string_view new_str, int count = -1);

/**
 * Split a string into multiple parts separated by `delim`.
 */
template<typename OutputIterator>
void SplitString(const std::string &text, OutputIterator iterator,
                 char delim = '\n') {
  std::stringstream ss(text);
  std::string item;

  while (std::getline(ss, item, delim)) {
    *iterator = item;
    ++iterator;
  }
}

/**
 * Returns a string which replaces all uppercase characters into lowercase.
 */
std::string ToLower(const std::string &s);

/**
 * Returns true if two strings are the same when case ignored.
 */
bool EqualIgnoreCase(std::string_view lhs, std::string_view rhs);

/**
 * Returns a string which escapes all control characters in bash.
 */
std::string EscapeForShellStyle(std::string_view raw);

/**
 * Returns a random string only contains alphabetic and numeric characters.
 */
std::string RandomAlphaNumString(uint32_t length = 32);

std::string Base64Encode(uint8_t *str, uint32_t len);

std::vector<uint8_t> Base64Decode(std::string_view str);

}  // namespace utils
}  // namespace jk

// fmt specialization {{{
namespace fmt {

template<typename T>
struct formatter<T, char,
                 typename std::enable_if<
                     std::is_base_of_v<jk::utils::Stringifiable, T>>::type> {
  template<typename ParseContext>
  typename ParseContext::iterator parse(
      ParseContext &ctx) {  // NOLINT(runtime/references)
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const T &v,
              FormatContext &ctx)  // NOLINT(runtime/references)
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", v.Stringify());
  }
};

template<typename T>
struct formatter<boost::optional<T>> {
  using value_type = boost::optional<T>;

  template<typename ParseContext>
  typename ParseContext::iterator parse(
      ParseContext &ctx) {  // NOLINT(runtime/references)
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const value_type &v,
              FormatContext &ctx)  // NOLINT(runtime/references)
      -> decltype(ctx.out()) {
    if (v) {
      return format_to(ctx.out(), "Optional({})", v.get());
    }

    return format_to(ctx.out(), "Optional()");
  }
};

template<typename T>
struct formatter<std::optional<T>> {
  using value_type = std::optional<T>;

  template<typename ParseContext>
  typename ParseContext::iterator parse(
      ParseContext &ctx) {  // NOLINT(runtime/references)
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const value_type &v,
              FormatContext &ctx)  // NOLINT(runtime/references)
      -> decltype(ctx.out()) {
    if (v) {
      return format_to(ctx.out(), "Optional({})", *v);
    }

    return format_to(ctx.out(), "Optional()");
  }
};

template<>
struct formatter<semver::version> {
  using value_type = semver::version;

  template<typename ParseContext>
  typename ParseContext::iterator parse(
      ParseContext &ctx) {  // NOLINT(runtime/references)
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const value_type &v,
              FormatContext &ctx)  // NOLINT(runtime/references)
      -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", v.to_string());
  }
};

}  // namespace fmt

// }}}

// vim: fdm=marker sw=2 ts=2
