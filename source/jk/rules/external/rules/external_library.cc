// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/rules/external_library.hh"

#include <string_view>

namespace jk::rules::external {

ExternalLibrary::ExternalLibrary(core::rules::BuildPackage *package,
                                 std::string name,
                                 std::string_view rule_type_name)
    : BuildRule(package, name, {core::rules::RuleTypeEnum::kExternal},
                rule_type_name) {
}

bool ExternalLibrary::IsStable() const {
  return true;
}

void ExternalLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  Url = kwargs.StringRequired("url");
  Sha256 = kwargs.StringRequired("sha256");
  ArchiveType = kwargs.StringRequired("type");
  OutputFile = kwargs.StringRequired("output");
  Anchors = kwargs.ListOptional("anchors", empty_list);
  auto it = kwargs.Find("header_only");
  if (it != kwargs.End()) {
    if (!it->second.get_type().is(pybind11::bool_().get_type())) {
      JK_THROW(core::JKBuildError("field 'header_only' expect bool"));
    }
    HeaderOnly = it->second.cast<bool>();
  } else {
    HeaderOnly = false;
  }
}

std::vector<std::string> ExternalLibrary::ExportedFilesSimpleName(
    core::filesystem::ProjectFileSystem *project,
    const std::string &build_type) const {
  return {};
}

std::vector<std::string> ExternalLibrary::ExportedLinkFlags() const {
  return {};
}

std::vector<std::string> ExternalLibrary::ExportedHeaders() const {
  return {};
}

}  // namespace jk::rules::external

// vim: fdm=marker
