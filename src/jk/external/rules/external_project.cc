// Copyright (c) 2020 Hawtian Wang
//

#include "jk/external/rules/external_project.hh"

#include "jk/core/rules/build_rule.hh"

namespace jk::external {

ExternalProject::ExternalProject(core::rules::BuildPackage *package,
                                 std::string name)
    : BuildRule(package, name,
                {core::rules::RuleTypeEnum::kLibrary,
                 core::rules::RuleTypeEnum::kExternal},
                "external_project") {
}

bool ExternalProject::IsStable() const {
  return true;
}

void ExternalProject::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
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

  LdFlags = kwargs.ListOptional("ldflags", empty_list);
  Headers = kwargs.ListOptional("headers", empty_list);
}

std::vector<std::string> ExternalProject::ExportedFilesSimpleName() const {
  return Exports;
}

std::vector<std::string> ExternalProject::ExportedLinkFlags() const {
  return LdFlags;
}

std::vector<std::string> ExternalProject::ExportedHeaders() const {
  return Headers;
}

}  // namespace jk::external

// vim: fdm=marker

