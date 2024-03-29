// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <ranges>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_join.h"
#include "absl/strings/strip.h"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/rule_type.hh"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/filter.hpp"

namespace jk::impls::rules {

static auto logger = utils::Logger("cc_library");

CCLibrary::CCLibrary(core::models::BuildPackage *package, utils::Kwargs kwargs,
                     std::string type_name, core::models::RuleType type)
    : BuildRule(package, std::move(type_name), type, package->Path.Stringify(),
                std::move(kwargs)) {
}

#define FILL_LIST_FIELD(field, key) field = kwargs.ListOptional(key, empty_list)

auto CCLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) -> void {
  static auto empty_list = std::make_optional<std::vector<std::string>>({});

  BuildRule::ExtractFieldFromArguments(kwargs);

  logger->debug("Extract fields, {}", Base->FullQualifiedName);

  FILL_LIST_FIELD(CFlags, "cflags");
  FILL_LIST_FIELD(CppFlags, "cppflags");
  FILL_LIST_FIELD(CxxFlags, "cxxflags");
  FILL_LIST_FIELD(LdFlags, "ldflags");
  FILL_LIST_FIELD(Sources, "srcs");
  FILL_LIST_FIELD(Excludes, "excludes");
  FILL_LIST_FIELD(Includes, "includes");
  FILL_LIST_FIELD(Defines, "defines");
  FILL_LIST_FIELD(Headers, "headers");
  FILL_LIST_FIELD(AlwaysCompile, "always_compile");

  if (Headers.empty()) {
    // NOTE(hawtian): for backward-compatibility Some files in library can't not
    // lint, but it passed. Because the old build system only check files with
    // suffix '.h' and '.inl'.
    Headers.push_back("*.h");
    Headers.push_back("**/*.h");
  }
}

auto CCLibrary::DoPrepare(core::models::Session *session) -> void {
  // super prepare
  BuildRule::DoPrepare(session);

  package_root_ = session->Project->Resolve(Package->Path.Path);

  LibraryFileName = fmt::format("lib{}.a", Base->Name);

  // step 1. prepare nolint files
  prepare_nolint_files(session);

  // step 2. excludes
  prepare_excludes(session);

  // step 3. headers
  prepare_header_files(session);

  // step 4. source files
  prepare_source_files(session);

  // step 5. source files
  prepare_always_compile_files(session);

  // step 6. link flags
  ExportedLinkFlags = LdFlags;

  // step 7. environment vars;
  for (auto tp : session->BuildTypes) {
    auto artifact = WorkingFolder.Sub(tp, LibraryFileName).Stringify();
    ExportedEnvironmentVars.emplace_back("artifact_" + tp, std::move(artifact));
  }
  ExportedEnvironmentVars.emplace_back("working_folder",
                                       WorkingFolder.Stringify());

  // step 8. cache flags
  ExpandedCFileFlags = ranges::views::concat(CFlags, CxxFlags) |
                       ranges::views::filter([](const auto &s) {
                         return !absl::StartsWith(s, "-I");
                       }) |
                       ranges::to<decltype(ExpandedCFileFlags)>();

  ExpandedCppFileFlags = ranges::views::concat(CppFlags, CxxFlags) |
                         ranges::views::filter([](const auto &s) {
                           return !absl::StartsWith(s, "-I");
                         }) |
                         ranges::to<decltype(ExpandedCFileFlags)>();

  // step 8. construct include and define flags
  prepare_include_flags(session);
  prepare_define_flags(session);
  prepare_inherent_flags(session);
}

auto CCLibrary::prepare_nolint_files(core::models::Session *session) -> void {
  auto nolint_txt = session->Project->Resolve(Package->Path.Path, "nolint.txt");
  if (!std::filesystem::exists(nolint_txt.Path)) {
    return;
  }

  NolintFiles.clear();
  std::string line;
  std::ifstream ifs(nolint_txt.Path.string());
  while (std::getline(ifs, line)) {
    absl::StripAsciiWhitespace(&line);
    auto expanded = session->PatternExpander->Expand(
        line, session->Project->Resolve(Package->Path.Path));

    for (const auto &f : expanded) {
      NolintFiles.insert(f);
    }
  }

  logger->debug("NOLINT files: [{}]", utils::JoinString(", ", NolintFiles));
}

auto CCLibrary::prepare_source_files(core::models::Session *session) -> void {
  ExpandedSourceFiles.clear();

  for (const auto &source : Sources) {
    auto expanded = session->PatternExpander->Expand(source, *package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        ExpandedSourceFiles.push_back(
            fs::relative(f, package_root_.value().Path).string());
      }
    }
  }

  std::sort(std::begin(ExpandedSourceFiles), std::end(ExpandedSourceFiles));
  logger->debug("SourceFiles in {}, from [{}] to [{}]", Base->StringifyValue,
                absl::StrJoin(Sources, ", "),
                absl::StrJoin(ExpandedSourceFiles, ", "));
}

void CCLibrary::prepare_excludes(core::models::Session *session) {
  excludes_.clear();

  for (const auto &exclude : Excludes) {
    auto expanded = session->PatternExpander->Expand(exclude, *package_root_);
    for (const auto &f : expanded) {
      excludes_.insert(f);
    }
  }
}

auto CCLibrary::prepare_header_files(core::models::Session *session) -> void {
  ExpandedHeaderFiles.clear();

  for (const auto &header : Headers) {
    auto expanded = session->PatternExpander->Expand(header, *package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        ExpandedHeaderFiles.push_back(
            fs::relative(f, package_root_.value().Path).string());
      }
    }
  }

  std::sort(std::begin(ExpandedHeaderFiles), std::end(ExpandedHeaderFiles));
  logger->debug("Headers in {}: [{}]", Base->StringifyValue,
                absl::StrJoin(ExpandedHeaderFiles, ", "));
}

auto CCLibrary::prepare_always_compile_files(core::models::Session *session)
    -> void {
  ExpandedAlwaysCompileFiles.clear();

  for (const auto &source : AlwaysCompile) {
    auto expanded = session->PatternExpander->Expand(source, *package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        ExpandedAlwaysCompileFiles.insert(f);
      }
    }
  }

  logger->debug("AlwaysCompile in {}: [{}]", Base->StringifyValue,
                absl::StrJoin(ExpandedAlwaysCompileFiles, ", "));
}

auto CCLibrary::prepare_include_flags(core::models::Session *session) -> void {
  absl::flat_hash_set<uint32_t> visisted;
  auto dfs = [this, &visisted, session](core::models::BuildRule *rule,
                                        auto &&dfs) {
    if (visisted.contains(rule->Base->ObjectId)) {
      return;
    }
    visisted.insert(rule->Base->ObjectId);
    if (auto cc_rule = dynamic_cast<CCLibrary *>(rule); cc_rule != nullptr) {
      for (const auto &s : ranges::views::concat(
               cc_rule->CppFlags, cc_rule->CFlags, cc_rule->CxxFlags)) {
        if (absl::StartsWith(s, "-I")) {
          ResolvedIncludes.insert(s);
        }
      }

      // fast-path for cc_library
      for (const auto &s : cc_rule->Includes) {
        ResolvedIncludes.insert(fmt::format("-I{}", s));
      }

      if (cc_rule->Base->Type.IsProto()) {
        // if proto, add its 'working_folder'

        ResolvedIncludes.insert(
            fmt::format("-I{}", session->Project->BuildRoot
                                    .Sub(cc_rule->Base->FullQuotedQualifiedName)
                                    .Stringify()));
      }
    } else {
      static std::vector<std::string> empty;

      for (const auto &s :
           rule->Base->_kwargs.ListOptional("includes", empty)) {
        ResolvedIncludes.insert(fmt::format("-I{}", s));
      }
    }

    for (auto dep : rule->Dependencies) {
      dfs(dep, dfs);
    }
  };

  ResolvedIncludes.insert("-I.");

  dfs(this, dfs);
}

auto CCLibrary::prepare_define_flags(core::models::Session *) -> void {
  absl::flat_hash_set<uint32_t> visisted;
  auto dfs = [this, &visisted](core::models::BuildRule *rule, auto &&dfs) {
    if (visisted.contains(rule->Base->ObjectId)) {
      return;
    }
    visisted.insert(rule->Base->ObjectId);
    auto cc_rule = dynamic_cast<CCLibrary *>(rule);
    if (cc_rule) {
      // fast-path for cc_library
      for (const auto &s : cc_rule->Defines) {
        ResolvedDefines.insert(fmt::format("-D{}", s));
      }
    } else {
      static std::vector<std::string> empty;
      for (const auto &s : rule->Base->_kwargs.ListOptional("defines", empty)) {
        ResolvedDefines.insert(fmt::format("-D{}", s));
      }
    }
    for (auto dep : rule->Dependencies) {
      dfs(dep, dfs);
    }
  };

  dfs(this, dfs);
}

auto CCLibrary::prepare_inherent_flags(core::models::Session *) -> void {
  absl::flat_hash_set<uint32_t> visisted;
  auto dfs = [this, &visisted](core::models::BuildRule *rule, auto &&dfs) {
    if (visisted.contains(rule->Base->ObjectId)) {
      return;
    }
    visisted.insert(rule->Base->ObjectId);
    ResolvedInherentFlags.insert(std::begin(rule->InherentFlags),
                                 std::end(rule->InherentFlags));
    for (auto dep : rule->Dependencies) {
      dfs(dep, dfs);
    }
  };

  dfs(this, dfs);
}

const std::vector<std::string> &CCLibrary::ExportedFiles(
    core::models::Session *session, std::string_view build_type) {
  tmp_exported_files_ = {
      WorkingFolder.Sub(build_type, LibraryFileName).Stringify()};
  return tmp_exported_files_;
}

}  // namespace jk::impls::rules
