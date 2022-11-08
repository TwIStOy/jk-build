// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "jk/core/models/build_rule.hh"
#include "jk/utils/assert.hh"
#include "jk/utils/cpp_features.hh"
#include "jk/utils/kwargs.hh"

namespace jk::core::models {

struct __JK_HIDDEN BuildRuleFactory {
  using create_func_t = std::function<std::unique_ptr<BuildRule>(
      BuildPackage *, utils::Kwargs kwargs)>;

  inline std::unique_ptr<BuildRule> Create(const std::string &TypeName,
                                           BuildPackage *pkg,
                                           utils::Kwargs kwargs) {
    if (auto it = creators_.find(TypeName); it != creators_.end()) {
      return (it->second)(pkg, std::move(kwargs));
    } else {
      utils::assertion::boolean.expect(false, "No creator");
      return nullptr;
    }
  }

  inline void AddCreator(const std::string &TypeName, create_func_t f) {
    creators_.emplace(TypeName, std::move(f));
  }

  template<typename R>
  inline void AddSimpleCreator(const std::string &TypeName) {
    creators_.emplace(TypeName, [](BuildPackage *pkg, utils::Kwargs kwargs) {
      return std::make_unique<R>(pkg, std::move(kwargs));
    });
  }

 private:
  absl::flat_hash_map<std::string, create_func_t> creators_;
};

}  // namespace jk::core::models
