// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/proto_library.hh"

#include <algorithm>
#include <unordered_set>
#include <utility>

#include "jk/core/rules/package.hh"

namespace jk::rules::cc {

static auto logger = utils::Logger("proto_library");

ProtoLibrary::ProtoLibrary(BuildPackage *package, std::string name)
    : CCLibrary(package, name, {RuleTypeEnum::kLibrary, RuleTypeEnum::kCC},
                "proto_library", fmt::format("lib{}.a", name)) {
  ExtraIncludes.push_back(IncludeArgument::Placehoder::WorkingFolder);
}

bool ProtoLibrary::IsStable() const {
  return true;
}

void ProtoLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  Sources = kwargs.ListRequired("srcs");
  Excludes = kwargs.ListOptional("excludes", empty_list);
}

std::vector<std::string> ProtoLibrary::ExportedFilesSimpleName(
    core::filesystem::JKProject *project, const std::string &build_type) const {
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
