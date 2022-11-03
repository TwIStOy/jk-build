// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <optional>
#include <utility>

namespace jk::utils {

template<typename T>
struct LazyEvaluatedValue {
 public:
  using eval_func_t = std::function<T()>;

  template<typename F>
  explicit LazyEvaluatedValue(F &&f) : eval_func_(std::forward<F>(f)) {
  }

  const T &Value() const & {
    if (!value_.has_value()) {
      value_ = eval_func_();
    }
    return value_.value();
  }

  const T &operator()() const & {
    return Value();
  }

  T &Value() & {
    if (!value_.has_value()) {
      value_ = eval_func_();
    }
    return value_.value();
  }

  void Clear() & {
    value_.reset();
  }

 private:
  [[no_unique_address]] eval_func_t eval_func_;
  mutable std::optional<T> value_;
};

}  // namespace jk::utils

// vim: fdm=marker
