// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "jk/common/lazy_property.hh"
#include "jk/core/models/rule_type.hh"
#include "jk/utils/kwargs.hh"

namespace jk::core::models {

// All threse fields can be load from BUILD.
struct BuildRuleInfoStatic {
  BuildRuleInfoStatic(std::string TypeName, RuleType type, std::string Name,
                      std::string_view PackageName, std::string Version);

  BuildRuleInfoStatic(const BuildRuleInfoStatic &)            = delete;
  BuildRuleInfoStatic(BuildRuleInfoStatic &&)                 = delete;
  BuildRuleInfoStatic &operator=(const BuildRuleInfoStatic &) = delete;
  BuildRuleInfoStatic &operator=(BuildRuleInfoStatic &&)      = delete;

  // [[auto-generate]]
  const uint32_t ObjectId;

  // [[rule-function]]
  const std::string TypeName;

  // [[rule-function]]
  const RuleType Type;

  //! The rule's name. Name must be unique in a package.
  // [[arg: `name`]]
  const std::string Name;

  //! The rule's package's name.
  // [[rule-file-name]]
  const std::string_view PackageName;

  //! The rule's version. For backward compatible, default is 'DEFAULT'.
  // [[arg: `version`]]
  const std::string Version;

  // [[arg: `deps`]]
  const std::vector<std::string> Dependencies;
};

using FullQualifiedName_t =
    common::LazyProperty<std::string,
                         decltype([](BuildRuleInfoStatic *self) -> std::string {
                           return fmt::format("{}/{}@{}", self->PackageName,
                                              self->Name, self->Version);
                         }),
                         BuildRuleInfoStatic>;

using FullQualifiedNameWithoutVersion_t =
    common::LazyProperty<std::string,
                         decltype([](BuildRuleInfoStatic *self) -> std::string {
                           return fmt::format("{}/{}", self->PackageName,
                                              self->Name);
                         }),
                         BuildRuleInfoStatic>;

struct _BuildRuleBase : BuildRuleInfoStatic {
  using BuildRuleInfoStatic::BuildRuleInfoStatic;

  //! The rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  FullQualifiedName_t FullQualifiedName{this};

  //! The rule's full qualifed name without version.
  FullQualifiedNameWithoutVersion_t FullQualifiedNameWithoutVersion{this};

  common::LazyProperty<std::string,
                       decltype([](BuildRuleInfoStatic *self) -> std::string {
                         return fmt::format(R"(<Rule:{} "{}:{}">)",
                                            self->TypeName, self->PackageName,
                                            self->Name);
                       }),
                       BuildRuleInfoStatic>
      StringifyValue{this};
};

using FullQuotedQualifiedName_t =
    common::LazyProperty<std::string, decltype([](_BuildRuleBase *self) {
                           std::string res;
                           for (auto ch : self->FullQualifiedName.Value()) {
                             if (ch == '/') {
                               res.push_back('@');
                               res.push_back('@');
                             } else {
                               res.push_back(ch);
                             }
                           }
                           return res;
                         }),
                         _BuildRuleBase>;

using FullQuotedQualifiedNameWithoutVersion_t =
    common::LazyProperty<std::string, decltype([](_BuildRuleBase *self) {
                           std::string res;
                           for (auto ch :
                                self->FullQualifiedNameWithoutVersion.Value()) {
                             if (ch == '/') {
                               res.push_back('@');
                               res.push_back('@');
                             } else {
                               res.push_back(ch);
                             }
                           }
                           return res;
                         }),
                         _BuildRuleBase>;

struct BuildRuleBase : _BuildRuleBase {
  using _BuildRuleBase::BuildRuleInfoStatic;

  //! The rule's full quoted qualifed name. Replace all '/' to '@@'.
  FullQuotedQualifiedName_t FullQuotedQualifiedName{this};

  //! The rule's full quoted qualifed name without version. Repalce all '/' to
  //! '@@'.
  FullQuotedQualifiedNameWithoutVersion_t FullQuotedQualifiedNameWithoutVersion{
      this};
};

inline auto FromArguments(const utils::Kwargs &kwargs)
    -> std::tuple<std::string, std::string, std::vector<std::string>> {
  static std::vector<std::string> Empty;
  auto name    = kwargs.StringRequired("name");
  auto version = kwargs.StringOptional("version", "");
  auto deps    = kwargs.ListOptional("deps", Empty);
  return {std::move(name), std::move(version), std::move(deps)};
}

}  // namespace jk::core::models
