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
#include "jk/core/error.h"
#include "jk/core/models/rule_type.hh"
#include "jk/utils/kwargs.hh"
#include "jk/utils/logging.hh"

namespace jk::core::models {

uint32_t __CurrentObjectId();

// All threse fields can be load from BUILD.
struct BuildRuleBase {
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

  //! The rule's name. Name must be unique in a package.
  // [[arg: `name`]]
  std::string Name;

  //! The rule's package's name.
  // [[rule-file-name]]
  std::string_view PackageName;

  //! The rule's version. For backward compatible, default is 'DEFAULT'.
  // [[arg: `version`]]
  std::string Version;

  // [[arg: `deps`]]
  std::vector<std::string> Dependencies;

  //! The rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  std::string FullQualifiedName;

  //! The rule's full qualifed name without version.
  std::string FullQualifiedNameWithoutVersion;

  //! Cached stringify result
  std::string StringifyValue;

  //! The rule's full quoted qualifed name. Replace all '/' to '@@'.
  std::string FullQuotedQualifiedName;

  //! The rule's full quoted qualifed name without version. Repalce all '/' to
  //! '@@'.
  std::string FullQuotedQualifiedNameWithoutVersion;

  utils::Kwargs _kwargs;
};

}  // namespace jk::core::models
