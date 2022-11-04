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
  BuildRuleInfoStatic();
  BuildRuleInfoStatic(std::string TypeName, RuleType type, std::string Name,
                      std::string_view PackageName, std::string Version);

  BuildRuleInfoStatic(const BuildRuleInfoStatic &)            = delete;
  BuildRuleInfoStatic(BuildRuleInfoStatic &&)                 = delete;
  BuildRuleInfoStatic &operator=(const BuildRuleInfoStatic &) = delete;
  BuildRuleInfoStatic &operator=(BuildRuleInfoStatic &&)      = delete;

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

}  // namespace jk::core::models
