// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/rules/external_library.hh"

#include <string>
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
  // default: jk download {url} {output} {sha256} {columns}
  DownloadCommand = kwargs.ListOptional("download_command", empty_list);
  // default: ./configure --prefix=${JK_PROJECT_DIR}
  ConfigureCommand = kwargs.ListOptional("configure_command", empty_list);
  // default: $(MAKE)
  BuildCommand = kwargs.ListOptional("build_command", empty_list);
  // default: $(MAKE) install
  InstallCommand = kwargs.ListOptional("install_command", empty_list);
  // default: "DEFAULT" for backwards compatibility
  Version = kwargs.StringOptional("version", std::string("DEFAULT"));

  Libraries = kwargs.ListOptional("libraries", empty_list);
  LdFlags = kwargs.ListOptional("ldflags", empty_list);
}

std::vector<std::string> ExternalLibrary::ExportedFilesSimpleName(
    core::filesystem::JKProject *project, const std::string &build_type) const {
  if (Libraries.size()) {
    auto prefix = project->ExternalInstalledPrefix;
    std::vector<std::string> res;
    for (const auto &lib : Libraries) {
      fs::path p(lib);
      res.push_back(prefix.Sub(p).Stringify());
    }
    return res;
  }
  return {};
}

std::vector<std::string> ExternalLibrary::ExportedLinkFlags() const {
  return LdFlags;
}

std::vector<std::string> ExternalLibrary::ExportedHeaders() const {
  return {};
}

}  // namespace jk::rules::external

// vim: fdm=marker
