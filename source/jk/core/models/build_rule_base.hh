// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "fmt/format.h"
#include "jk/common/lazy_property.hh"
#include "jk/core/error.h"
#include "jk/core/models/rule_type.hh"
#include "jk/utils/kwargs.hh"
#include "jk/utils/logging.hh"

namespace jk::core::models {

uint32_t __CurrentObjectId();

// All threse fields can be load from BUILD.
struct __JK_HIDDEN BuildRuleBase {
  BuildRuleBase(std::string TypeName, RuleType type,
                std::string_view PackageName, utils::Kwargs kwargs);

  BuildRuleBase(const BuildRuleBase &)            = delete;
  BuildRuleBase(BuildRuleBase &&)                 = delete;
  BuildRuleBase &operator=(const BuildRuleBase &) = delete;
  BuildRuleBase &operator=(BuildRuleBase &&)      = delete;

  void FromArguments(const utils::Kwargs &kwargs);

  // [[auto-generate]]
  uint32_t ObjectId;

  // [[rule-function]]
  std::string TypeName;

  // [[rule-function]]
  RuleType Type;

  template<typename T>
  struct PropretyProxy {
    PropretyProxy(BuildRuleBase *base) : Base(base) {
    }

    T &operator[](const std::string &key) {
      if (auto it = properties.find(key); it != properties.end()) {
        return it->second;
      }
      if (auto it = Base->_kwargs.Find(key); it != Base->_kwargs.End()) {
        auto value       = it->second.cast<T>();
        auto [it2, succ] = properties.emplace(key, std::move(value));
        assert(succ);
        return it2->second;
      }
      JK_THROW(core::JKBuildError("expect field '{}' but not found", key));
    }

    template<typename F>
      requires std::constructible_from<T, std::invoke_result_t<F>>
    T &operator()(const std::string &key, F &&f) {
      if (auto it = properties.find(key); it != properties.end()) {
        return it->second;
      }
      if (auto it = Base->_kwargs.Find(key); it != Base->_kwargs.End()) {
        auto value       = it->second.cast<T>();
        auto [it2, succ] = properties.emplace(key, std::move(value));
        assert(succ);
        return it2->second;
      } else {
        auto [it2, succ] = properties.emplace(key, f());
        assert(succ);
        return it2->second;
      }
      JK_THROW(core::JKBuildError("expect field '{}' but not found", key));
    }

   private:
    BuildRuleBase *Base;

    absl::flat_hash_map<std::string, T> properties;
  };

  PropretyProxy<std::string> StrProperty;

  PropretyProxy<std::vector<std::string>> StrListProperty;

  //! The rule's name. Name must be unique in a package.
  // [[arg: `name`]]
  common::LazyProperty<std::string> Name;

  //! The rule's package's name.
  // [[rule-file-name]]
  std::string_view PackageName;

  //! The rule's version. For backward compatible, default is 'DEFAULT'.
  // [[arg: `version`]]
  common::LazyProperty<std::string> Version;

  // [[arg: `deps`]]
  common::LazyProperty<std::vector<std::string>> Dependencies;

  //! The rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  common::LazyProperty<std::string> FullQualifiedName;

  //! The rule's full qualifed name without version.
  common::LazyProperty<std::string> FullQualifiedNameWithoutVersion;

  //! Cached stringify result
  common::LazyProperty<std::string> StringifyValue;

  //! The rule's full quoted qualifed name. Replace all '/' to '@@'.
  common::LazyProperty<std::string> FullQuotedQualifiedName;

  //! The rule's full quoted qualifed name without version. Repalce all '/' to
  //! '@@'.
  common::LazyProperty<std::string> FullQuotedQualifiedNameWithoutVersion;

 private:
  utils::Kwargs _kwargs;
};

}  // namespace jk::core::models
