// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>

namespace jk::common {

template<typename T>
class LazyProperty {
 public:
  using func_t    = std::function<T()>;
  using reference = std::add_lvalue_reference_t<T>;

  explicit LazyProperty(func_t func = nullptr) : func_(std::move(func)) {
  }

  const T &Value() const & {
    if (value_.has_value()) [[unlikely]] {
      return value_.value();
    }
    value_.emplace(func_());
  }

  T &Value() & {
    if (value_.has_value()) [[unlikely]] {
      return value_.value();
    }
    value_.emplace(func_());
  }

  const T &operator*() const & {
    return Value();
  }

  T &operator*() & {
    return Value();
  }

  T const *operator->() const & {
    return &value_.value();
  }

  T *operator->() & {
    return &value_.value();
  }

  bool has_func() const {
    return func_ != nullptr;
  }

  template<typename F>
  LazyProperty &operator=(F &&f) {
    Reset(std::forward<F>(f));
  }

  void Reset(func_t func) {
    value_.reset();
    func_ = std::move(func);
  }

  void ForceReload() {
    value_.emplace(func_());
  }

 private:
  mutable std::optional<T> value_;
  [[no_unique_address]] func_t func_;
};

}  // namespace jk::common
