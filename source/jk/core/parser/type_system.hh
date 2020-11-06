// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <tuple>
#include <type_traits>
#include <variant>

#include "jk/utils/type_traits.hh"

namespace jk::core::parser {

template<typename T>
struct __variant_to_typelist {
  using type = utils::type_list<T>;
};

template<typename... Args>
struct __variant_to_typelist<std::variant<Args...>> {
  using type = utils::type_list<Args...>;
};

template<typename T, typename U>
struct TypeOr {
  using _type = typename utils::type_list<
      typename __variant_to_typelist<T>::type,
      typename __variant_to_typelist<U>::type>::flatten::unique::variant;

  using type = std::conditional_t<std::variant_size_v<_type> == 1,
                                  std::variant_alternative<0, _type>, _type>;
};

template<typename T, typename U>
struct TypePlus;

}  // namespace jk::core::parser

// vim: fdm=marker

