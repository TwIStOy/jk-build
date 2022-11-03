// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_rule_base.hh"

#include <atomic>
#include <string>
#include <string_view>

namespace jk::core::models {

static std::atomic_uint_fast32_t CurrentObjectId = 0;

BuildRuleBase::BuildRuleBase(std::string type_name, RuleType type,
                             std::string name, std::string_view package_name,
                             std::string version)
    : ObjectId(CurrentObjectId.fetch_add(1)),
      TypeName(std::move(type_name)),
      Type(type),
      Name(std::move(name)),
      PackageName(package_name),
      Version(std::move(version)),
      FullQualifiedName([this] {
        return fmt::format("{}/{}@{}", PackageName, Name, Version);
      }),
      FullQuotedQualifiedName([this] {
        std::string res;
        for (auto ch : FullQualifiedName.Value()) {
          if (ch == '/') {
            res.push_back('@');
            res.push_back('@');
          } else {
            res.push_back(ch);
          }
        }
        return res;
      }),
      FullQualifiedNameWithoutVersion([this] {
        return fmt::format("{}/{}", PackageName, Name);
      }),
      FullQuotedQualifiedNameWithoutVersion([this] {
        std::string res;
        for (auto ch : FullQualifiedNameWithoutVersion.Value()) {
          if (ch == '/') {
            res.push_back('@');
            res.push_back('@');
          } else {
            res.push_back(ch);
          }
        }
        return res;
      }),
      stringify_value_([this] {
        return fmt::format(R"(<Rule:{} "{}:{}">)", TypeName, PackageName, Name);
      }) {
}

}  // namespace jk::core::models
