// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_rule_base.hh"

#include <atomic>
#include <list>
#include <string>
#include <string_view>
#include <vector>

#include "jk/common/lazy_property.hh"

namespace jk::core::models {

static std::atomic_uint_fast32_t CurrentObjectId = 0;

uint32_t __CurrentObjectId() {
  return CurrentObjectId;
}

BuildRuleBase::BuildRuleBase(std::string type_name, RuleType type,
                             std::string_view package_name,
                             utils::Kwargs kwargs)
    : ObjectId(CurrentObjectId.fetch_add(1)),
      TypeName(std::move(type_name)),
      Type(type),
      PackageName(package_name),
      StrProperty(this),
      StrListProperty(this),
      _kwargs(std::move(kwargs)) {
  Name = [this] {
    return _kwargs.StringRequired("name");
  };

  Version = [this] {
    return _kwargs.StringOptional("version", "");
  };

  Dependencies = [this] {
    return _kwargs.ListOptional("deps", {});
  };

  FullQualifiedName = [this]() {
    return fmt::format("{}/{}@{}", PackageName, *Name, *Version);
  };

  FullQualifiedNameWithoutVersion = [this]() -> std::string {
    return fmt::format("{}/{}", PackageName, *Name);
  };

  StringifyValue = [this]() -> std::string {
    return fmt::format(R"(<Rule:{} "{}:{}">)", TypeName, PackageName, *Name);
  };

  FullQuotedQualifiedName = [this]() -> std::string {
    std::string res;
    for (auto ch : *FullQualifiedName) {
      if (ch == '/') {
        res.push_back('_');
      } else {
        res.push_back(ch);
      }
    }
    return res;
  };

  FullQuotedQualifiedNameWithoutVersion = [this]() {
    std::string res;
    for (auto ch : *FullQualifiedNameWithoutVersion) {
      if (ch == '/') {
        res.push_back('_');
      } else {
        res.push_back(ch);
      }
    }
    return res;
  };
}

}  // namespace jk::core::models
