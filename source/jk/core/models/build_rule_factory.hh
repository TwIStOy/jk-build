// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <functional>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "jk/core/models/build_rule.hh"
#include "jk/utils/kwargs.hh"

namespace jk::core::models {

struct BuildRuleFactory {
  using create_func_t = std::function<std::unique_ptr<BuildRule>(
      const std::string &TypeName, const utils::Kwargs &kwargs)>;

  inline std::unique_ptr<BuildRule> Create(const std::string &TypeName,
                                           utils::Kwargs kwargs) {
    if (auto it = creators_.find(TypeName); it != creators_.end()) {
      return (it->second)(TypeName, std::move(kwargs));
    } else {
      return nullptr;
    }
  }

  inline void AddCreator(const std::string &TypeName, create_func_t f) {
    creators_.emplace(TypeName, std::move(f));
  }

 private:
  absl::flat_hash_map<std::string, create_func_t> creators_;
};

}  // namespace jk::core::models
