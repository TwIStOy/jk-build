// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <fmt/format.h>

#include <boost/optional.hpp>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace jk {

using fmt::operator"" _format;

namespace utils {

struct Stringifiable {
  virtual std::string Stringify() const = 0;

  virtual ~Stringifiable() = default;
};

inline bool StringEndWith(std::string_view full_string,
                          std::string_view ending) {
  if (full_string.length() >= ending.length()) {
    return (0 == full_string.compare(full_string.length() - ending.length(),
                                     ending.length(), ending));
  } else {
    return false;
  }
}

template<typename InputIterator>
typename InputIterator::value_type __DefaultUnary(
    const typename InputIterator::value_type &v) {
  return v;
}

template<
    typename InputIterator,
    typename UnaryOperator = decltype(__DefaultUnary<InputIterator>),
    typename = typename std::iterator_traits<InputIterator>::iterator_category>
inline std::string JoinString(
    std::string separator, InputIterator begin, InputIterator end,
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

template<typename Container,
         typename UnaryOperator =
             decltype(__DefaultUnary<typename Container::const_iterator>),
         typename = decltype(std::declval<Container>().begin(),
                             std::declval<Container>().end())>
inline std::string JoinString(
    std::string separator, Container container,
    UnaryOperator func = &__DefaultUnary<typename Container::const_iterator>) {
  return JoinString(std::move(separator), std::begin(container),
                    std::end(container), func);
}

template<typename T>
inline std::string Replace(const std::string &old, char from, const T &to) {
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

std::string EscapeForShellStyle(const std::string &raw);

}  // namespace utils
}  // namespace jk

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

}  // namespace fmt
