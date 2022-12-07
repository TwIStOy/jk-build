// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <type_traits>

#include "jk/core/models/build_package_factory.hh"
#include "range/v3/range/traits.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::core::models {

inline auto IterAllRules(BuildPackageFactory *factory) {
  return factory->IterPackages() | ranges::views::transform([](auto pkg) {
           return pkg->IterRules();
         }) |
         ranges::views::join;
}

static_assert(std::same_as<ranges::range_value_t<std::invoke_result_t<
                               decltype(IterAllRules), BuildPackageFactory *>>,
                           BuildRule *>);

}  // namespace jk::core::models
