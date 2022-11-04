// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_library.hh"

#include <glob.h>

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "boost/optional/optional.hpp"
#include "fmt/core.h"
#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/rules/cc/include_argument.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

static auto logger = utils::Logger("cc_library");

CCLibrary::CCLibrary(BuildPackage *package, std::string name,
                     std::initializer_list<RuleTypeEnum> types,
                     std::string_view type_name, std::string exported_file_name)
    : BuildRule(package, name, std::move(types), type_name),
      ExportedFileName(exported_file_name.empty() ? fmt::format("lib{}.a", name)
                                                  : exported_file_name) {
}


std::vector<std::string> CCLibrary::ResolveIncludes(
    IncludesResolvingContext *ctx) {
  std::vector<std::string> res;

  std::unordered_set<std::string> recorder;
  RecursiveExecute(
      [&res, ctx](BuildRule *_self) {
        if (!_self->Type.HasType(RuleTypeEnum::kLibrary)) {
          return;
        }
        auto self = dynamic_cast<CCLibrary *>(_self);
        std::copy(std::begin(self->Includes), std::end(self->Includes),
                  std::back_inserter(res));
        std::transform(
            std::begin(self->ExtraIncludes), std::end(self->ExtraIncludes),
            std::back_inserter(res),
            [self, ctx](const IncludeArgument &x) -> std::string {
              if (x.IsTrivial()) {
                return x.StrValue();
              } else {
                switch (x.PlacehoderValue()) {
                  case IncludeArgument::Placehoder::WorkingFolder: {
                    return self->WorkingFolder(ctx->Project()->BuildRoot);
                  }
                  default: {
                    assert(false);
                    JK_THROW("not impl");
                  }
                }
              }
            });
      },
      &recorder);

  std::sort(res.begin(), res.end());
  res.erase(std::unique(res.begin(), res.end()), res.end());

  return res;
}

const std::vector<std::string> &CCLibrary::ResolveDefinitions() const {
  if (resolved_definitions_) {
    return resolved_definitions_.value();
  }

  std::unordered_set<std::string> recorder;
  return ResolveDefinitionsImpl(&recorder);
}

const std::vector<std::string> &CCLibrary::ResolveDefinitionsImpl(
    std::unordered_set<std::string> *recorder) const {
  const static std::vector<std::string> empty = {};
  if (resolved_definitions_) {
    return resolved_definitions_.value();
  }

  if (auto it = recorder->find(FullQualifiedName()); it != recorder->end()) {
    return empty;
  }
  recorder->insert(FullQualifiedName());

  std::vector<std::string> res;
  res.insert(res.end(), Defines.begin(), Defines.end());

  for (const auto &dep : Dependencies) {
    if (dep->Type.HasType(RuleTypeEnum::kLibrary)) {
      auto tmp = dep->Downcast<CCLibrary>()->ResolveDefinitionsImpl(recorder);
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }
  std::sort(res.begin(), res.end());
  res.erase(std::unique(res.begin(), res.end()), res.end());

  logger->debug("BuildRule: {}, ResolvedDefinitions: [{}]", FullQualifiedName(),
                utils::JoinString(", ", res.begin(), res.end()));

  resolved_definitions_ = std::move(res);
  return resolved_definitions_.value();
}

const std::vector<std::string> &CCLibrary::FlagsForCppFiles() const {
  if (resolved_cpp_flags_) {
    return resolved_cpp_flags_.value();
  }

  std::vector<std::string> res;
  res.insert(res.end(), CppFlags.begin(), CppFlags.end());
  res.insert(res.end(), CxxFlags.begin(), CxxFlags.end());

  resolved_cpp_flags_ = std::move(res);
  return resolved_cpp_flags_.value();
}

const std::vector<std::string> &CCLibrary::FlagsForCFiles() const {
  if (resolved_c_flags_) {
    return resolved_c_flags_.value();
  }

  std::vector<std::string> res;
  res.insert(res.end(), CFlags.begin(), CFlags.end());
  res.insert(res.end(), CxxFlags.begin(), CxxFlags.end());

  resolved_c_flags_ = std::move(res);
  return resolved_c_flags_.value();
}

bool CCLibrary::IsNolint(const std::string &name) const {
  if (!nolint_files_) {
    return false;
  }
  return nolint_files_->count(name) > 0;
}

std::vector<std::string> CCLibrary::ExportedFilesSimpleName(
    core::filesystem::JKProject *project, const std::string &build_type) const {
  return {WorkingFolder(project->BuildRoot)
              .Sub(build_type)
              .Sub(ExportedFileName)
              .Stringify()};
}

std::vector<std::string> CCLibrary::ExportedLinkFlags() const {
  return LdFlags;
}

std::unordered_map<std::string, std::string> CCLibrary::ExportedEnvironmentVar(
    core::filesystem::JKProject *project) const {
  std::unordered_map<std::string, std::string> res;

  for (auto tp : common::FLAGS_BuildTypes) {
    res["artifact_" + tp] =
        utils::JoinString(" ", ExportedFilesSimpleName(project, tp));
  }

  return res;
}

}  // namespace jk::rules::cc
