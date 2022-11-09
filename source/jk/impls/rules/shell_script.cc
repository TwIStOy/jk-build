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
    if (it->second.value.index() == 0) {
      Exports = std::vector<std::string>{std::get<0>(it->second.value)};
      break;
    }
    if (it->second.value.index() == 1) {
      Exports.clear();

      for (const auto &x : std::get<1>(it->second.value)) {
        Exports.push_back(std::get<0>(x->value));
      }
      break;
    }

    JK_THROW(core::JKBuildError("field '{}' expect type list or str, but {}",
                                "export", it->second.value.index()));
  } while (0);
  do {
    auto it = kwargs.Find("export_bin");
    if (it == kwargs.End()) {
      break;
    }
    if (it->second.value.index() != 2) {
      JK_THROW(core::JKBuildError("field '{}' expect type dict", "export_bin"));
    }

    ExportBin.clear();
    for (auto [k, v] : std::get<2>(it->second.value)) {
      ExportBin[k] = std::get<0>(v->value);
    }
  } while (0);

  LdFlags = kwargs.ListOptional("ldflags", empty_list);
}

auto ShellScript::DoPrepare(core::models::Session *session) -> void {
  BuildRule::DoPrepare(session);

  Artifacts =
      Exports | ranges::views::transform([&](const std::string &lib) {
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

auto ShellScript::ExportedFiles(core::models::Session *session,
                                std::string_view build_type)
    -> const std::vector<std::string> & {
  return Artifacts;
}

}  // namespace jk::impls::rules
