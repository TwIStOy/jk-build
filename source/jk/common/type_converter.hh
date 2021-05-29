// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <type_traits>
#include <utility>

namespace jk::common {

template<typename T>
struct IInto {
  using target_type = T;

  virtual ~IInto() = default;

  virtual target_type Into() = 0;
};

template<typename T, typename U, typename = void>
struct is_from_impl : std::false_type {};

template<typename T, typename U>
struct is_from_impl<T, U, decltype(T::From(std::declval<U>()))>
    : std::true_type {};

template<typename target_type, typename from_type>
target_type jk_cast(from_type &&from) {
  static_assert(
      std::is_base_of_v<IInto<target_type>, std::decay_t<from_type>> ||
          is_from_impl<target_type, from_type>::value,
      "Inhert Into or impl From required.");

  if constexpr (std::is_base_of_v<IInto<target_type>,
                                  std::decay_t<from_type>>) {
    return from.Into();
  }

  if constexpr (is_from_impl<target_type, from_type>::value) {
    return target_type::From(std::forward<from_type>(from));
  }
}

}  // namespace jk::common

// vim: fdm=marker
