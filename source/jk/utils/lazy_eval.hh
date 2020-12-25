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

  const T &Value() {
    if (!value_.has_value()) {
      value_ = eval_func_();
    }
    return value_.value();
  }

 private:
  eval_func_t eval_func_;
  std::optional<T> value_;
};

}  // namespace jk::utils

// vim: fdm=marker
