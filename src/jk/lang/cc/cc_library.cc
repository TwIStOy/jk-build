// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/cc_library.hh"

#include <algorithm>
#include <boost/optional/optional.hpp>
#include <iterator>
#include <string>
#include <vector>

#include "fmt/core.h"
#include "jk/core/rules/build_rule.hh"

namespace jk {
namespace core {
namespace rules {

CCLibrary::CCLibrary(BuildPackage *package, std::string name,
                     std::initializer_list<RuleTypeEnum> types,
                     std::string_view type_name)
    : BuildRule(package, name, std::move(types), type_name),
      ExportedFileName(fmt::format("lib{}.a", name)) {
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
  res.insert(res.end(), Includes.begin(), Includes.end());

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
  auto &includes = ResolveIncludes();
  std::transform(includes.begin(), includes.end(), std::back_inserter(res),
                 [](const std::string &inc) {
                   return fmt::format("-I{}", inc);
                 });

  auto &defines = ResolveDefinitions();
  std::transform(defines.begin(), defines.end(), std::back_inserter(res),
                 [](const std::string &inc) {
                   return fmt::format("-D{}", inc);
                 });

  resolved_cpp_flags_ = std::move(res);
  return resolved_cpp_flags_.get();
}

const std::vector<std::string> &CCLibrary::FlagsForCFiles() const {
  if (resolved_c_flags_) {
    return resolved_c_flags_.get();
  }

  std::vector<std::string> res;
  res.insert(res.end(), CFlags.begin(), CFlags.end());
  auto &includes = ResolveIncludes();
  std::transform(includes.begin(), includes.end(), std::back_inserter(res),
                 [](const std::string &inc) {
                   return fmt::format("-I{}", inc);
                 });

  auto &defines = ResolveDefinitions();
  std::transform(defines.begin(), defines.end(), std::back_inserter(res),
                 [](const std::string &inc) {
                   return fmt::format("-D{}", inc);
                 });

  resolved_c_flags_ = std::move(res);
  return resolved_c_flags_.get();
}

}  // namespace rules
}  // namespace core
}  // namespace jk

