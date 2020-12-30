// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <list>
#include <memory>
#include <utility>

#include "args.hxx"
#include "boost/optional.hpp"

namespace jk::cli {

struct VariableGroup {
  args::Group *group;

  void Notify();
  std::list<std::function<void()>> callbacks_;
};

template<typename T>
struct ArgsReader : args::ValueReader {};

template<typename T, typename Reader = ArgsReader<T>>
class Variable {
 public:
  using value_type = T;
  using FlagType =
      typename std::conditional<std::is_same<T, bool>::value, args::Flag,
                                args::ValueFlag<T, args::ValueReader>>::type;

  Variable() {
  }

  template<typename U, typename = std::enable_if<!std::is_same<
                           typename std::decay<U>::type, Variable>::value>>
  explicit Variable(U &&v) {
    Value = std::forward<U>(v);
  }

  template<typename... Ts>
  void Register(VariableGroup *group, Ts &&...v) {
    flag_.reset(new FlagType(*group->group, std::forward<Ts>(v)...));

    group->callbacks_.push_back([this]() {
      if (*flag_) {
        Value = args::get(*flag_);
      }
    });
  }

  boost::optional<T> Value;

 private:
  std::unique_ptr<FlagType> flag_{nullptr};
};

}  // namespace jk::cli

// vim: fdm=marker
