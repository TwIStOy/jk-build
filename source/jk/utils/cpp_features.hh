// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

namespace jk::utils {

#define ALWAYS_INLINE inline __attribute__((always_inline))

template<typename... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};

template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace jk::utils

// vim: fdm=marker
