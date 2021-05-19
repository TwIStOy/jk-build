// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

namespace jk::common {

template<typename T>
struct Into {
  using target_type = T;

  virtual ~Into() = default;

  virtual target_type into() = 0;
};

template<typename T, typename U>
struct From {
  using from_type = T;
  using target_type = U;

  virtual ~From() = default;

  virtual target_type from(from_type rhs) = 0;
};

template<typename target_type, typename from_type>
target_type custom_cast(from_type &&from) {
}

}  // namespace jk::common

// vim: fdm=marker
