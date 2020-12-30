// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "boost/type_traits.hpp"

namespace jk::utils::functools {

template<typename F, std::size_t MaxKeys>
class __MemoImpl;

template<typename R, typename... Args, std::size_t MaxKeys>
class __MemoImpl<std::function<R(Args...)>, MaxKeys> {
 public:
  using func_t = std::function<R(Args...)>;
  using result_t = R;
  using arguments_t = std::tuple<Args...>;
  using key_t = std::tuple<std::decay_t<Args>...>;

  template<typename _F,
           typename = std::enable_if_t<std::is_convertible_v<_F &&, func_t>>>
  __MemoImpl(_F &&f) : func_(std::forward<_F>(f)) {
  }

  template<typename... Ts>
  R operator()(Ts &&...args) {
    return func_(std::forward<Ts>(args)...);
  }

 private:
  func_t func_;
};

template<std::size_t MaxKeys = 16, typename... Fs>
class Memo : public __MemoImpl<Fs, MaxKeys>... {
 public:
  template<typename... Ts>
  Memo(Ts &&...funcs) : __MemoImpl<Fs, MaxKeys>(std::forward<Ts>(funcs))... {
    static_assert(sizeof...(Fs) == sizeof...(Ts));
  }
};

}  // namespace jk::utils::functools

// vim: fdm=marker
