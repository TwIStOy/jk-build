// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <memory>
#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"
#include "jk/core/interfaces/compiler.hh"

namespace jk::impls::compilers {

struct CompilerFactory {
 public:
  inline core::interfaces::Compiler *Find(std::string_view generator_name,
                                          std::string_view rule_type) const {
    if (auto it = compilers_.find(generator_name); it != compilers_.end()) {
      if (auto it2 = it->second.find(rule_type); it2 != it->second.end()) {
        return it2->second.get();
      }
    }
    return nullptr;
  }

  template<typename C>
  inline void Register(std::string_view generator_name,
                       std::string_view rule_type) {
    compilers_[generator_name][rule_type].reset(new C);
  }

 private:
  absl::flat_hash_map<
      std::string,
      absl::flat_hash_map<std::string,
                          std::unique_ptr<core::interfaces::Compiler>>>
      compilers_;
};

}  // namespace jk::impls::compilers
