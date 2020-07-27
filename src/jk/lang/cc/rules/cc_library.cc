// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/rules/cc_library.hh"

#include <glob.h>

#include <algorithm>
#include <boost/optional/optional.hpp>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

#include "fmt/core.h"
#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/lang/cc/source_file.hh"

namespace jk {
namespace core {
namespace rules {

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

const std::vector<std::string> &CCLibrary::ResolveIncludes() const {
  if (resolved_includes_) {
    return resolved_includes_.get();
  }

  std::vector<std::string> res;
  res.insert(res.end(), Includes.begin(), Includes.end());

  for (const auto &dep : Dependencies) {
    if (dep->Type.HasType(RuleTypeEnum::kLibrary)) {
      auto tmp = dep->Downcast<CCLibrary>()->ResolveIncludes();
      res.insert(res.end(), tmp.begin(), tmp.end());
    }
  }
  std::sort(res.begin(), res.end());
  res.erase(std::unique(res.begin(), res.end()), res.end());

  resolved_includes_ = std::move(res);
  return resolved_includes_.get();
}

const std::vector<std::string> &CCLibrary::ResolveDefinitions() const {
  if (resolved_definitions_) {
    return resolved_definitions_.get();
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

  resolved_definitions_ = std::move(res);
  return resolved_definitions_.get();
}

const std::vector<std::string> &CCLibrary::FlagsForCppFiles() const {
  if (resolved_cpp_flags_) {
    return resolved_cpp_flags_.get();
  }

  std::vector<std::string> res;
  res.insert(res.end(), CppFlags.begin(), CppFlags.end());
  res.insert(res.end(), CxxFlags.begin(), CxxFlags.end());
  // auto &includes = ResolveIncludes();
  // std::transform(includes.begin(), includes.end(), std::back_inserter(res),
  //                [](const std::string &inc) {
  //                  return fmt::format("-I{}", inc);
  //                });
  //
  // auto &defines = ResolveDefinitions();
  // std::transform(defines.begin(), defines.end(), std::back_inserter(res),
  //                [](const std::string &inc) {
  //                  return fmt::format("-D{}", inc);
  //                });

  resolved_cpp_flags_ = std::move(res);
  return resolved_cpp_flags_.get();
}

const std::vector<std::string> &CCLibrary::FlagsForCFiles() const {
  if (resolved_c_flags_) {
    return resolved_c_flags_.get();
  }

  std::vector<std::string> res;
  res.insert(res.end(), CFlags.begin(), CFlags.end());
  res.insert(res.end(), CxxFlags.begin(), CxxFlags.end());
  // auto &includes = ResolveIncludes();
  // std::transform(includes.begin(), includes.end(), std::back_inserter(res),
  //                [](const std::string &inc) {
  //                  return fmt::format("-I{}", inc);
  //                });
  //
  // auto &defines = ResolveDefinitions();
  // std::transform(defines.begin(), defines.end(), std::back_inserter(res),
  //                [](const std::string &inc) {
  //                  return fmt::format("-D{}", inc);
  //                });

  resolved_c_flags_ = std::move(res);
  return resolved_c_flags_.get();
}

const std::vector<std::string> &CCLibrary::ExpandSourceFiles(
    filesystem::ProjectFileSystem *project,
    filesystem::FileNamePatternExpander *expander) const {
  if (expanded_source_files_) {
    return expanded_source_files_.get();
  }

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

  expanded_source_files_ = std::move(result);

  return expanded_source_files_.get();
}

std::vector<std::string> CCLibrary::ExportedFilesSimpleName(
    filesystem::ProjectFileSystem *project,
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

}  // namespace rules
}  // namespace core
}  // namespace jk
