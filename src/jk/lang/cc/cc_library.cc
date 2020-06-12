// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/cc_library.hh"

#include <boost/optional/optional.hpp>

#include "fmt/core.h"
#include "jk/core/rules/build_rule.hh"

namespace jk {
namespace core {
namespace rules {

CCLibrary::CCLibrary(BuildPackage *package, std::string name,
                     std::initializer_list<RuleTypeEnum> types,
                     std::string_view type_name)
    : BuildRule(package, name, std::move(types), type_name),
      ExportedFileName(fmt::format("lib{}.a", name)) {
}

bool CCLibrary::IsStable() const {
  return false;
}

#define FILL_LIST_FIELD(field, key) field = kwargs.ListOptional(key, empty_list)

void CCLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  // clang-format off
  FILL_LIST_FIELD(CFlags,   "cflags");
  FILL_LIST_FIELD(CppFlags, "cppflags");
  FILL_LIST_FIELD(CxxFlags, "cxxflags");
  FILL_LIST_FIELD(LdFlags,  "ldflags");
  FILL_LIST_FIELD(Sources,  "srcs");
  FILL_LIST_FIELD(Excludes, "excludes");
  FILL_LIST_FIELD(Includes, "includes");
  FILL_LIST_FIELD(Defines,  "defines");
  FILL_LIST_FIELD(Headers,  "headers");
  // clang-format on
}

}  // namespace rules
}  // namespace core
}  // namespace jk

