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

void ShellScript::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  core::rules::BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = std::make_optional<std::vector<std::string>>({});

  Script = kwargs.StringRequired("script");
  do {
    auto it = kwargs.Find("export");
    if (it == kwargs.End()) {
      JK_THROW(core::JKBuildError("expect field '{}' but not found", "export"));
    }
    if (pybind11::isinstance<pybind11::str>(it->second)) {
      Exports = std::vector<std::string>{it->second.cast<std::string>()};
      break;
    }
    if (pybind11::isinstance<pybind11::list>(it->second)) {
      Exports =
          std::vector<std::string>{it->second.cast<utils::Kwargs::ListType>()};
      break;
    }

    JK_THROW(core::JKBuildError(
        "field '{}' expect type list or str, but {}", "export",
        pybind11::str(it->second.get_type()).cast<std::string>()));
  } while (0);
  do {
    auto it = kwargs.Find("export_bin");
    if (it == kwargs.End()) {
      break;
    }
    if (!pybind11::isinstance<pybind11::dict>(it->second)) {
      JK_THROW(core::JKBuildError("field '{}' expect type dict", "export_bin"));
    }

    ExportBin.clear();
    for (auto k : it->second) {
      if (!pybind11::isinstance<pybind11::str>(k)) {
        JK_THROW(core::JKBuildError("key in field '{}' expect type str",
                                    "export_bin"));
      }
    }
    ExportBin = it->second.cast<std::unordered_map<std::string, std::string>>();
  } while (0);

  LdFlags = kwargs.ListOptional("ldflags", empty_list);
}

std::vector<std::string> ShellScript::ExportedFilesSimpleName(
    core::filesystem::JKProject *project, const std::string &build_type) const {
  (void)build_type;
  std::vector<std::string> res;

  std::transform(std::begin(Exports), std::end(Exports),
                 std::back_inserter(res), [&](const std::string &p) {
                   return project->ProjectRoot.Sub(".build")
                       .Sub(".lib")
                       .Sub(fmt::format("m{}", ToString(project->Platform)))
                       .Sub("lib")
                       .Sub(p)
                       .Stringify();
                 });

  return res;
}

std::vector<std::string> ShellScript::ExportedLinkFlags() const {
  return LdFlags;
}

std::unordered_map<std::string, std::string>
ShellScript::ExportedEnvironmentVar(core::filesystem::JKProject *) const {
  return ExportBin;
}

}  // namespace jk::rules::external

// vim: fdm=marker
