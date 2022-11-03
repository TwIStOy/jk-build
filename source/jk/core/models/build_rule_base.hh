// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <string>
#include <string_view>

#include "fmt/format.h"
#include "jk/core/models/rule_type.hh"
#include "jk/utils/lazy_eval.hh"

namespace jk::core::models {

// All threse fields can be load from BUILD.
struct BuildRuleBase {
  BuildRuleBase(std::string TypeName, RuleType type, std::string Name,
                std::string_view PackageName, std::string Version);

  BuildRuleBase(const BuildRuleBase &)            = delete;
  BuildRuleBase(BuildRuleBase &&)                 = delete;
  BuildRuleBase &operator=(const BuildRuleBase &) = delete;
  BuildRuleBase &operator=(BuildRuleBase &&)      = delete;

  const uint32_t ObjectId;

  const std::string TypeName;

  const RuleType Type;

  //! The rule's name. Name must be unique in a package.
  const std::string Name;

  //! The rule's package's name.
  const std::string_view PackageName;

  //! The rule's version. For backward compatible, default is 'DEFAULT'.
  const std::string Version;

  //! The rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  utils::LazyEvaluatedValue<std::string> FullQualifiedName;

  //! The rule's full quoted qualifed name. Replace all '/' to '@@'.
  utils::LazyEvaluatedValue<std::string> FullQuotedQualifiedName;

  //! The rule's full qualifed name without version.
  utils::LazyEvaluatedValue<std::string> FullQualifiedNameWithoutVersion;

  //! The rule's full quoted qualifed name without version. Repalce all '/' to
  //! '@@'.
  utils::LazyEvaluatedValue<std::string> FullQuotedQualifiedNameWithoutVersion;

 private:
  utils::LazyEvaluatedValue<std::string> stringify_value_;
};

}  // namespace jk::core::models
