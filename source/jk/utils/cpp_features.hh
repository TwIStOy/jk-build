// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

namespace jk::utils {

#define __JK_ALWAYS_INLINE inline __attribute__((always_inline))

#define __JK_HIDDEN __attribute__((visibility("hidden")))

#define __JK_FWD(n) std::forward<decltype(n)>(n)

template<typename... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};

template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace jk::utils

// vim: fdm=marker
