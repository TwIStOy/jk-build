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

bool CCLibrary::IsStable() const {
  return false;
}

#define FILL_LIST_FIELD(field, key) field = kwargs.ListOptional(key, empty_list)

void CCLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  BuildRule::ExtractFieldFromArguments(kwargs);

  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  // clang-format off
  FILL_LIST_FIELD(CFlags,   "cflags");
  FILL_LIST_FIELD(CppFlags, "cppflags");
  FILL_LIST_FIELD(CxxFlags, "cxxflags");
  FILL_LIST_FIELD(LdFlags,  "ldflags");
  FILL_LIST_FIELD(Sources,  "srcs");
  FILL_LIST_FIELD(Excludes, "excludes");
  FILL_LIST_FIELD(Includes, "includes");
  FILL_LIST_FIELD(Defines,  "defines");
  FILL_LIST_FIELD(Headers,  "headers");
  // clang-format on
}

const std::vector<IncludeArgument> &CCLibrary::ResolveIncludesInternal() const {
  if (resolved_includes_) {
    return resolved_includes_.value();
  }

  std::vector<IncludeArgument> res;
  std::transform(std::begin(Includes), std::end(Includes),
                 std::back_inserter(res), [](const auto &x) {
                   return IncludeArgument{x};
                 });
  // res.insert(res.end(), std::begin(ExtraIncludes), std::end(ExtraIncludes));

  for (const auto &dep : Dependencies) {
    if (dep->Type.HasType(RuleTypeEnum::kLibrary)) {
      auto tmp = dep->Downcast<CCLibrary>()->ResolveIncludesInternal();
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }
  std::sort(res.begin(), res.end());
  res.erase(std::unique(res.begin(), res.end()), res.end());

  logger->debug("BuildRule: {}, ResolvedIncludes: [{}]", FullQualifiedName(),
                utils::JoinString(", ", res.begin(), res.end()));

  resolved_includes_ = std::move(res);
  return resolved_includes_.value();
}

std::vector<std::string> CCLibrary::ResolveIncludes(
    IncludesResolvingContext *ctx) const {
  std::vector<std::string> res;

  std::copy(std::begin(Includes), std::end(Includes), std::back_inserter(res));
  std::transform(std::begin(ExtraIncludes), std::end(ExtraIncludes),
                 std::back_inserter(res),
                 [this, ctx](const IncludeArgument &x) -> std::string {
                   if (x.IsTrivial()) {
                     return x.StrValue();
                   } else {
                     switch (x.PlacehoderValue()) {
                       case IncludeArgument::Placehoder::WorkingFolder: {
                         return WorkingFolder(ctx->Project()->BuildRoot);
                       }
                     }
                   }
                 });

  for (const auto &dep : Dependencies) {
    if (dep->Type.HasType(RuleTypeEnum::kLibrary)) {
      auto tmp = dep->Downcast<CCLibrary>()->ResolveIncludes(ctx);
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }

  std::sort(res.begin(), res.end());
  res.erase(std::unique(res.begin(), res.end()), res.end());

  return res;
}

const std::vector<std::string> &CCLibrary::ResolveDefinitions() const {
  if (resolved_definitions_) {
    return resolved_definitions_.value();
  }

  std::vector<std::string> res;
  res.insert(res.end(), Defines.begin(), Defines.end());

  for (const auto &dep : Dependencies) {
    if (dep->Type.HasType(RuleTypeEnum::kLibrary)) {
      auto tmp = dep->Downcast<CCLibrary>()->ResolveDefinitions();
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

void CCLibrary::LoadNolintFiles(
    core::filesystem::ProjectFileSystem *project,
    core::filesystem::FileNamePatternExpander *expander) const {
  if (nolint_files_) {
    return;
  }

  auto p = project->Resolve(Package->Path).Sub("nolint.txt");
  if (!boost::filesystem::exists(p.Path)) {
    return;
  }

  std::unordered_set<std::string> nolint_files;
  std::string line;
  std::ifstream ifs(p.Path.string());
  while (std::getline(ifs, line)) {
    boost::algorithm::trim(line);
    auto expanded = expander->Expand(line, project->Resolve(Package->Path));
    for (const auto &f : expanded) {
      nolint_files.insert(f);
    }
  }

  logger->info("NOLINT files: [{}]", utils::JoinString(", ", nolint_files));

  nolint_files_ = std::move(nolint_files);
}

bool CCLibrary::IsNolint(const std::string &name) const {
  if (!nolint_files_) {
    return false;
  }
  return nolint_files_->count(name) > 0;
}

const std::vector<std::string> &CCLibrary::ExpandSourceFiles(
    core::filesystem::ProjectFileSystem *project,
    core::filesystem::FileNamePatternExpander *expander) const {
  if (expanded_source_files_) {
    return expanded_source_files_.value();
  }

  LoadNolintFiles(project, expander);

  std::vector<std::string> result;
  std::unordered_set<std::string> excludes;

  fs::path package_root = Package->Name;
  for (const auto &exclude : Excludes) {
    auto expanded = expander->Expand(exclude, project->Resolve(Package->Path));
    for (const auto &f : expanded) {
      excludes.insert(f);
    }
  }

  for (const auto &source : Sources) {
    auto expanded = expander->Expand(source, project->Resolve(Package->Path));

    for (const auto &f : expanded) {
      if (excludes.find(f) == excludes.end()) {
        result.push_back(
            fs::relative(f, project->Resolve(Package->Path).Path).string());
      }
    }
  }

  std::sort(std::begin(result), std::end(result));
  logger->info(
      "SourceFiles in {}: [{}]", *this,
      utils::JoinString(", ", std::begin(result), std::end(result),
                        [](const std::string &filename) -> std::string {
                          return fmt::format(R"("{}")", filename);
                        }));

  expanded_source_files_ = std::move(result);

  return expanded_source_files_.value();
}

std::vector<std::string> CCLibrary::ExportedFilesSimpleName(
    core::filesystem::ProjectFileSystem *project,
    const std::string &build_type) const {
  return {WorkingFolder(project->BuildRoot)
              .Sub(build_type)
              .Sub(ExportedFileName)
              .Stringify()};
}

std::vector<std::string> CCLibrary::ExportedLinkFlags() const {
  return LdFlags;
}

std::vector<std::string> CCLibrary::ExportedHeaders() const {
  std::vector<std::string> res;
  std::transform(Headers.begin(), Headers.end(), std::back_inserter(res),
                 [this](const std::string &hdr) {
                   return Package->Path.Sub(hdr).Stringify();
                 });
  return Headers;
}

}  // namespace jk::rules::cc
