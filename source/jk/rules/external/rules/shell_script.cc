// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/rules/shell_script.hh"

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/common/flags.hh"
#include "jk/core/rules/build_rule.hh"

namespace jk::rules::external {

ShellScript::ShellScript(core::rules::BuildPackage *package, std::string name)
    : BuildRule(package, name, {core::rules::RuleTypeEnum::kExternal},
                "shell_script") {
}

bool ShellScript::IsStable() const {
  return true;
}

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
      Exports = std::vector<std::string>{it->second.cast<std::string>()};
      break;
    }
    if (it->second.get_type().is(pybind11::list().get_type())) {
      Exports =
          std::vector<std::string>{it->second.cast<utils::Kwargs::ListType>()};
      break;
    }

    JK_THROW(
        core::JKBuildError("field '{}' expect type list or str", "export"));
  } while (0);
  do {
    auto it = kwargs.Find("export_bin");
    if (it == kwargs.End()) {
      break;
    }
    if (!it->second.get_type().is(pybind11::dict().get_type())) {
      JK_THROW(core::JKBuildError("field '{}' expect type dict", "export_bin"));
    }

    ExportBin.clear();
    for (auto k : it->second) {
      if (!k.get_type().is(pybind11::str().get_type())) {
        JK_THROW(core::JKBuildError("key in field '{}' expect type str",
                                    "export_bin"));
      }
    }
    ExportBin = it->second.cast<std::unordered_map<std::string, std::string>>();
  } while (0);

  LdFlags = kwargs.ListOptional("ldflags", empty_list);
  Headers = kwargs.ListOptional("headers", empty_list);
}

std::vector<std::string> ShellScript::ExportedFilesSimpleName(
    core::filesystem::JKProject *project, const std::string &build_type) const {
  (void)build_type;
  std::vector<std::string> res;

  std::transform(
      std::begin(Exports), std::end(Exports), std::back_inserter(res),
      [&](const std::string &p) {
        return project->ProjectRoot.Sub(".build")
            .Sub(".lib")
            .Sub(fmt::format(
                "m{}",
                common::FLAGS_platform == common::Platform::k32 ? 32 : 64))
            .Sub("lib")
            .Sub(p)
            .Stringify();
      });

  return res;
}

std::vector<std::string> ShellScript::ExportedLinkFlags() const {
  return LdFlags;
}

std::vector<std::string> ShellScript::ExportedHeaders() const {
  return Headers;
}

std::unordered_map<std::string, std::string>
ShellScript::ExportedEnvironmentVar(core::filesystem::JKProject *) const {
  return ExportBin;
}

}  // namespace jk::rules::external

// vim: fdm=marker
