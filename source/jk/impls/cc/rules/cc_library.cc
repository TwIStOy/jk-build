// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/cc/rules/cc_library.hh"

#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_join.h"
#include "absl/strings/strip.h"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_rule.hh"

namespace jk::impls::cc {

static auto logger = utils::Logger("cc_library");

#define FILL_LIST_FIELD(field, key) field = kwargs.ListOptional(key, empty_list)

auto CCLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) -> void {
  static auto empty_list = std::make_optional<std::vector<std::string>>({});

  core::models::BuildRule::ExtractFieldFromArguments(kwargs);

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

auto CCLibrary::Prepare(core::models::Session *session) -> void {
  // super prepare
  core::models::BuildRule::Prepare(session);

  package_root_ = session->Project->Resolve(Package->Path.Path);

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
}

auto CCLibrary::prepare_nolint_files(core::models::Session *session) -> void {
  auto nolint_txt = session->Project->Resolve(Package->Path.Path, "nolint.txt");
  if (!std::filesystem::exists(nolint_txt.Path)) {
    return;
  }

  nolint_files_.clear();
  std::string line;
  std::ifstream ifs(nolint_txt.Path.string());
  while (std::getline(ifs, line)) {
    absl::StripAsciiWhitespace(&line);
    auto expanded = session->PatternExpander->Expand(
        line, session->Project->Resolve(Package->Path.Path));

    for (const auto &f : expanded) {
      nolint_files_.insert(f);
    }
  }

  logger->debug("NOLINT files: [{}]", utils::JoinString(", ", nolint_files_));
}

auto CCLibrary::prepare_source_files(core::models::Session *session) -> void {
  expanded_source_files_.clear();

  for (const auto &source : Sources) {
    auto expanded = session->PatternExpander->Expand(source, package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        expanded_source_files_.push_back(
            fs::relative(f, package_root_.Path).string());
      }
    }
  }

  std::sort(std::begin(expanded_source_files_),
            std::end(expanded_source_files_));
  logger->debug("SourceFiles in {}: [{}]", Base->StringifyValue,
                absl::StrJoin(expanded_source_files_, ", "));
}

void CCLibrary::prepare_excludes(core::models::Session *session) {
  excludes_.clear();

  for (const auto &exclude : Excludes) {
    auto expanded = session->PatternExpander->Expand(exclude, package_root_);
    for (const auto &f : expanded) {
      excludes_.insert(f);
    }
  }
}

auto CCLibrary::prepare_header_files(core::models::Session *session) -> void {
  expanded_header_files_.clear();

  for (const auto &header : Headers) {
    auto expanded = session->PatternExpander->Expand(header, package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        expanded_header_files_.push_back(
            fs::relative(f, package_root_.Path).string());
      }
    }
  }

  std::sort(std::begin(expanded_header_files_),
            std::end(expanded_header_files_));
  logger->debug("Headers in {}: [{}]", Base->StringifyValue,
                absl::StrJoin(expanded_header_files_, ", "));
}

auto CCLibrary::prepare_always_compile_files(core::models::Session *session)
    -> void {
  always_compile_files_.clear();

  for (const auto &source : AlwaysCompile) {
    auto expanded = session->PatternExpander->Expand(source, package_root_);

    for (const auto &f : expanded) {
      if (!excludes_.contains(f)) {
        always_compile_files_.push_back(f);
      }
    }
  }

  std::sort(std::begin(always_compile_files_), std::end(always_compile_files_));
  logger->debug("AlwaysCompile in {}: [{}]", *Base->StringifyValue,
                absl::StrJoin(always_compile_files_, ", "));
}

}  // namespace jk::impls::cc
