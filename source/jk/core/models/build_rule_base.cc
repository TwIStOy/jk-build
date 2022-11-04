// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_rule_base.hh"

#include <atomic>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace jk::core::models {

static std::atomic_uint_fast32_t CurrentObjectId = 0;

BuildRuleInfoStatic::BuildRuleInfoStatic(std::string type_name, RuleType type,
                                         std::string name,
                                         std::string_view package_name,
                                         std::string version)
    : ObjectId(CurrentObjectId.fetch_add(1)),
      TypeName(std::move(type_name)),
      Type(type),
      Name(std::move(name)),
      PackageName(package_name),
      Version(std::move(version)) {
}

BuildRuleInfoStatic::BuildRuleInfoStatic()
    : ObjectId(CurrentObjectId.fetch_add(1)) {
}

void BuildRuleInfoStatic::FromArguments(const utils::Kwargs &kwargs) {
  static std::vector<std::string> Empty;

  Name         = kwargs.StringRequired("name");
  Version      = kwargs.StringOptional("version", "");
  Dependencies = kwargs.ListOptional("deps", Empty);
}

}  // namespace jk::core::models
