// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/shell_script.hh"

#include <string>

#include "jk/core/models/build_package.hh"
#include "jk/core/models/session.hh"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/algorithm/transform.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::impls::rules {

ShellScript::ShellScript(core::models::BuildPackage *package,
                         utils::Kwargs kwargs, std::string type_name,
                         core::models::RuleType type)
    : BuildRule(package, std::move(type_name), type, package->Path.Stringify(),
                std::move(kwargs)) {
}

auto ShellScript::ExtractFieldFromArguments(const utils::Kwargs &kwargs)
    -> void {
  BuildRule::ExtractFieldFromArguments(kwargs);

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

auto ShellScript::DoPrepare(core::models::Session *session) -> void {
  Artifacts =
      ranges::views::all(Exports) |
      ranges::views::transform([&](const std::string &lib) {
        return session->Project->ProjectRoot
            .Sub(".build", ".lib",
                 fmt::format("m{}", ToString(session->Project->Platform)),
                 "lib", lib)
            .Stringify();
      }) |
      ranges::to_vector;

  ExportedLinkFlags = LdFlags;

  for (const auto &pr : ExportBin) {
    ExportedEnvironmentVars.push_back(pr);
  }
}

}  // namespace jk::impls::rules
