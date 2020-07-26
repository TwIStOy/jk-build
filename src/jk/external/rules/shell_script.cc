// Copyright (c) 2020 Hawtian Wang
//

#include "jk/external/rules/shell_script.hh"

#include <algorithm>
#include <iterator>
#include <vector>

#include "fmt/core.h"
#include "jk/core/rules/build_rule.hh"
#include "jk/utils/logging.hh"

namespace jk::external {

bool ShellScript::IsStable() const {
  return true;
}

// script = "install_rdkafka.py",
// export = ["librdkafka++.a", "librdkafka.a"],
void ShellScript::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  core::rules::BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  Script = kwargs.StringRequired("script");
  do {
    auto it = kwargs.Find("export");
    if (it == kwargs.End()) {
      JK_THROW(core::JKBuildError("expect field '{}' but not found", "export"));
    }
    if (it->second.get_type().is(pybind11::str().get_type())) {
      Export = std::vector<std::string>{it->second.cast<std::string>()};
      break;
    }
    if (it->second.get_type().is(pybind11::list().get_type())) {
      Export =
          std::vector<std::string>{it->second.cast<utils::Kwargs::ListType>()};
      break;
    }

    JK_THROW(
        core::JKBuildError("field '{}' expect type list or str", "export"));
  } while (0);

  LdFlags = kwargs.ListOptional("ldflags", empty_list);
}

std::vector<std::string> ShellScript::ExportedFilesSimpleName() const {
  std::vector<std::string> res;

  std::transform(Export.begin(), Export.end(), std::back_inserter(res),
                 [](const std::string &s) {
                   return fmt::format("{}/{}/{}", "${JK_SOURCE_DIR}",
                                      ".build/.lib/m${PLATFORM}/lib/", s);
                 });

  return res;
}

}  // namespace jk::external

// vim: fdm=marker

