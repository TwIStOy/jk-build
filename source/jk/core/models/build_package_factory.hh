// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "jk/common/path.hh"
#include "jk/core/models/build_package.hh"
#include "range/v3/view/map.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::core::models {

class BuildPackageFactory {
 public:
  //! Get a *BuildPackage* from its address. This function will assume that
  //! any two calls with same argument will return the a same object.
  std::pair<BuildPackage *, bool /* new */> Package(std::string_view name) {
    std::unique_lock lk(mutex_);
    return PackageUnsafe(name);
  }

  std::pair<BuildPackage *, bool /* new */> PackageUnsafe(
      std::string_view name) {
    if (auto it = packages_.find(name); it != packages_.end()) {
      return {it->second.get(), false};
    } else {
      // TODO(hawtian): construct package
      auto new_package = std::make_unique<BuildPackage>(
          std::string{name}, common::ProjectRelativePath{fs::path(name)});
      auto ptr = new_package.get();
      packages_.emplace(name, std::move(new_package));
      return {ptr, true};
    }
  }

  auto IterPackages() {
    return packages_ | ranges::views::values |
           ranges::views::transform([](auto &x) {
             return x.get();
           });
  }

 private:
  std::mutex mutex_;
  absl::flat_hash_map<std::string, std::unique_ptr<BuildPackage>> packages_;
};

}  // namespace jk::core::models
