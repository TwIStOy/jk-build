// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <tuple>
#include <type_traits>
#include <variant>
namespace jk::utils {

template<typename... _types>
struct type_list;

template<typename T>
struct is_type_list : std::false_type {};

template<typename... Args>
struct is_type_list<type_list<Args...>> : std::true_type {};

template<typename T>
constexpr bool is_type_list_v = is_type_list<T>::value;

template<template<typename> class Func, typename T>
struct apply {
  using type = typename Func<T>::type;
};

template<template<typename> class Func, typename... Args>
struct apply<Func, type_list<Args...>> {
  using type = type_list<typename apply<Func, Args>::type...>;
};

template<typename T>
struct is_tuple : std::false_type {};

template<typename... args>
struct is_tuple<std::tuple<args...>> : std::true_type {};

template<typename T>
static constexpr bool is_tuple_v = is_tuple<T>::value;

namespace test {
static_assert(is_tuple_v<bool> == false, "");

using test_tp = std::tuple<int>;
static_assert(is_tuple_v<test_tp>, "");
}  // namespace test

namespace __detail {

template<typename... T>
struct __front_helper {
  static_assert(sizeof...(T) > 0);

  template<typename T0, typename...>
  struct __impl {
    using type = T0;
  };

  using type = typename __impl<T...>::type;
};

template<typename... T>
struct __pop_front_helper {
  static_assert(sizeof...(T) > 0);

  template<typename T0, typename... args>
  struct __impl {
    using type = type_list<args...>;
  };

  using type = typename __impl<T...>::type;
};

// template<typename T>
// struct __try_to_flatten {
//   using type = std::conditional_t<is_type_list_v<T>, T, type_list<T>>;
// };

template<typename previous, typename T>
struct __extend_push {
  using type = typename previous::template push_back<T>;
};

template<typename... Args0, typename... Args1>
struct __extend_push<type_list<Args0...>, type_list<Args1...>> {
  using type = type_list<Args0..., Args1...>;
};

template<typename now, typename... Args>
struct __unique_helper;

template<typename now, typename T0, typename... Rest>
struct __unique_helper<now, T0, Rest...> {
  using push_this = std::conditional_t<now::template in_v<T0>, now,
                                       typename now::template push_back<T0>>;
  using type = typename __unique_helper<push_this, Rest...>::type;
};

template<typename now, typename T0>
struct __unique_helper<now, T0> {
  using type = std::conditional_t<now::template in_v<T0>, now,
                                  typename now::template push_back<T0>>;
};

template<typename... Ts>
struct __flatten_helper;

template<>
struct __flatten_helper<> {
  using type = type_list<>;
};

template<typename T, typename... Rest>
struct __flatten_helper<T, Rest...> {
  using type = typename __flatten_helper<Rest...>::type::template push_front<T>;
};

template<typename... Ts, typename... Rest>
struct __flatten_helper<type_list<Ts...>, Rest...> {
  using first_part = typename __flatten_helper<Ts...>::type;
  using second_part = typename __flatten_helper<Rest...>::type;
  using type = typename __extend_push<first_part, second_part>::type;
};

}  // namespace __detail

template<>
struct type_list<> {
  using rebind = type_list<>;

  template<typename T>
  using push_back = type_list<T>;

  template<typename T>
  using push_front = type_list<T>;

  template<typename T>
  using in = std::false_type;

  template<typename T>
  static constexpr bool in_v = in<T>::value;

  using variant = std::variant<>;

  using flatten = rebind;

  using unique = rebind;
};

template<typename... _types>
struct type_list {
  using rebind = type_list<_types...>;

  using front = typename __detail::__front_helper<_types...>::type;

  using pop_front = typename __detail::__pop_front_helper<_types...>::type;

  template<typename T>
  using push_back = type_list<_types..., T>;

  template<typename T>
  using push_front = type_list<T, _types...>;

  template<typename T>
  using in = std::conditional_t<
      sizeof...(_types) == 0, std::false_type,
      std::conditional_t<std::is_same_v<T, front>, std::true_type,
                         typename pop_front::template in<T>>>;

  template<typename T>
  static constexpr bool in_v = in<T>::value;

  using variant = std::variant<_types...>;

  using flatten = typename __detail::__flatten_helper<_types...>::type;

  using unique =
      typename __detail::__unique_helper<type_list<>, _types...>::type;
};

}  // namespace jk::utils

// vim: fdm=marker

