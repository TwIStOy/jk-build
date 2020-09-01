// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/proto_library.hh"

namespace jk::rules::cc {

ProtoLibrary::ProtoLibrary(BuildPackage *package, std::string name)
    : BuildRule(package, name, {RuleTypeEnum::kLibrary, RuleTypeEnum::kCC},
                "proto_library"),
      ExportedFileName(fmt::format("lib{}.a", name)) {
}

bool ProtoLibrary::IsStable() const {
  return true;
}

void ProtoLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  BuildRule::ExtractFieldFromArguments(kwargs);

  Sources = kwargs.ListRequired("srcs");
}

std::vector<std::string> ProtoLibrary::ExportedFilesSimpleName(
    core::filesystem::ProjectFileSystem *project,
    const std::string &build_type) const {
  return {WorkingFolder(project->BuildRoot)
              .Sub(build_type)
              .Sub(ExportedFileName)
              .Stringify()};
}

std::vector<std::string> ProtoLibrary::ExportedLinkFlags() const {
  return {};
}

std::vector<std::string> ProtoLibrary::ExportedHeaders() const {
  return {};
}

}  // namespace jk::rules::cc

// vim: fdm=marker

