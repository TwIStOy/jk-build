// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/dependent.hh"

#include <string>
#include <string_view>
#include <utility>

#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

std::string BuildRuleId::Stringify() const {
  return "BuildRuleId(Id = \"{}:{}\", Position: {})"_format(PackageName,
                                                            RuleName, Position);
}

static uint32_t ReadPackageName(std::string_view str, std::string *res) {
  auto ed = 0;

  bool escaped = false;
  while (ed < str.length() && (escaped || str[ed] != ':')) {
    if (escaped) {
      escaped = false;
      res->push_back(str[ed]);
    } else if (str[ed] == '\\') {
      escaped = true;
    } else {
      res->push_back(str[ed]);
    }

    ed++;
  }

  if (utils::StringEndWith(*res, "/BUILD")) {
    // erase suffix
    *res = res->substr(0, res->length() - 6);
  }

  return ed;
}

BuildRuleId ParseIdString(std::string_view str) {
  BuildRuleId res;

  if (str.empty()) {
    JK_THROW(JKBuildError("Failed to parse dependent identifier '{}'.", str));
  }

  auto st = 0;
  if (str.find_first_of("//") == 0) {
    // start with '//'
    res.Position = RuleRelativePosition::kAbsolute;
    st += 2;
  } else if (str.find_first_of("##") == 0) {
    res.Position = RuleRelativePosition::kBuiltin;
    st += 2;
  } else if (str.find_first_of(":") == 0) {
    res.Position = RuleRelativePosition::kThis;
  } else {
    res.Position = RuleRelativePosition::kRelative;
  }

  std::string package_name;
  auto mid = ReadPackageName(str.substr(st), &package_name) + st;
  if (mid >= str.length()) {
    JK_THROW(JKBuildError("Failed to parse dep id '{}', unknown error", str));
  }
  if (str[mid] != ':') {
    JK_THROW(
        JKBuildError("Failed to parse dep id '{}', expect ':' at pos {} but "
                     "found '{}'",
                     str, mid, static_cast<char>(str[mid])));
  }

  if (!package_name.empty()) {
    res.PackageName = std::move(package_name);
  }
  res.RuleName = str.substr(mid + 1);

  return res;
}

}  // namespace rules
}  // namespace core
}  // namespace jk

