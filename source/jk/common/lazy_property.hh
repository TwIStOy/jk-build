// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <optional>
#include <tuple>
#include <type_traits>

namespace jk::common {

template<typename T, typename F, typename O>
  requires std::invocable<F, O *> &&
           std::constructible_from<T, std::invoke_result_t<F, O *>>
class LazyProperty {
 public:
  using reference = std::add_lvalue_reference_t<T>;

  explicit LazyProperty(O *pa) : pa_(pa) {
  }

  const T &Value() const & {
    if (value_.has_value()) [[unlikely]] {
      return value_.value();
    }
    value_.emplace(F{}(pa_));
  }

  T &Value() & {
    if (value_.has_value()) [[unlikely]] {
      return value_.value();
    }
    value_.emplace(F{}(pa_));
  }

  const T &operator*() const & {
    return Value();
  }

  T &operator*() & {
    return Value();
  }

 private:
  mutable std::optional<T> value_;
  O *pa_;
};

}  // namespace jk::common
