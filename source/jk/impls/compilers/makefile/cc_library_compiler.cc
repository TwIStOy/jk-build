// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/cc_library_compiler.hh"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "jk/common/path.hh"
#include "jk/core/generators/makefile.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/rules/cc_library.hh"
#include "range/v3/algorithm/contains.hpp"
#include "range/v3/all.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

static auto logger = utils::Logger("compiler.makefile.cc_library");

static std::string FixCpp20Gcc2ClangFlags(const std::string &flag) {
  if (flag == "-fcoroutines") {
    return "-fcoroutines-ts";
  }
  return flag;
}

static std::string FixCpp20Clang2GccFlags(const std::string &flag) {
  if (flag == "-fcoroutines-ts") {
    return "-fcoroutines";
  }
  return flag;
}

auto CCLibraryCompiler::Name() const -> std::string_view {
  return "makefile.cc_library";
}

auto CCLibraryCompiler::Compile(core::models::Session *session,
                                core::models::BuildRule *rule) const -> void {
  auto cc = dynamic_cast<rules::CCLibrary *>(rule);
  DoCompile(session, cc);
}

auto CCLibraryCompiler::DoCompile(core::models::Session *session,
                                  rules::CCLibrary *rule) const -> void {
  // TODO(hawtian): impl
}

#define WORKING_FOLDER  "WORKING_FOLDER"
#define CFLAGS          "CFLAGS"
#define CPPFLAGS        "CPPFLAGS"
#define CXXFLAGS        "CXXFLAGS"
#define CFLAGS_SUFFIX   "_CFLAGS"
#define CXXFLAGS_SUFFIX "_CXXFLAGS"
#define CPP_INCLUDES    "CPP_INCLUDES"
#define CPP_DEFINES     "CPP_DEFINES"

void generate_flag_file(core::models::Session *session,
                        const common::AbsolutePath &working_folder,
                        rules::CCLibrary *rule) {
  static auto git_desc =
      R"(-DGIT_DESC="\"`cd {} && git describe --tags --always`\"")";
  core::generators::Makefile makefile(working_folder.Sub("flags.make"));

  auto compile_flags = session->Config->compile_flags;
  compile_flags.push_back(
      fmt::format(git_desc, rule->Package->Path.Stringify()));

  // environments
  makefile.Env(WORKING_FOLDER, working_folder.Stringify());

  makefile.Env(CFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CPPFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CXXFLAGS,
               absl::StrJoin(ranges::views::concat(
                                 ranges::views::all(rule->CxxFlags),
                                 ranges::views::all(session->ExtraFlags)),
                             " "));

  auto cppincludes = ranges::views::concat(
      ranges::views::single("-isystem"),
      ranges::views::single(fmt::format(
          ".build/.lib/m{}/include",
          session->Project->Platform == core::filesystem::TargetPlatform::k64
              ? "64"
              : "32")),
      ranges::views::single("-I.build/include"));

  auto cxxincludes = ranges::views::single("-I.build/pb/c++");

  makefile.Env(
      "DEBUG" CFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Config->cflags,
                        session->Config->debug_cflags_extra, cppincludes),
                    " "));
  makefile.Env("DEBUG" CXXFLAGS_SUFFIX,
               absl::StrJoin(ranges::views::concat(
                                 compile_flags, session->Config->cxxflags,
                                 session->Config->debug_cxxflags_extra,
                                 cppincludes, cxxincludes),
                             " "));

  makefile.Env(
      "RELEASE" CFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Config->cflags,
                        session->Config->release_cflags_extra, cppincludes),
                    " "));
  makefile.Env("RELEASE" CXXFLAGS_SUFFIX,
               absl::StrJoin(ranges::views::concat(
                                 compile_flags, session->Config->cxxflags,
                                 session->Config->release_cxxflags_extra,
                                 cppincludes, cxxincludes),
                             " "));

  makefile.Env(
      "PROFILING" CFLAGS_SUFFIX,
      absl::StrJoin(ranges::views::concat(
                        compile_flags, session->Config->cflags,
                        session->Config->profiling_cflags_extra, cppincludes),
                    " "));
  makefile.Env("PROFILING" CXXFLAGS_SUFFIX,
               absl::StrJoin(ranges::views::concat(
                                 compile_flags, session->Config->cxxflags,
                                 session->Config->profiling_cxxflags_extra,
                                 cppincludes, cxxincludes),
                             " "));

  makefile.Env(CPP_DEFINES, absl::StrJoin(rule->ResolvedDefines, " "));

  makefile.Env(CPP_INCLUDES, absl::StrJoin(rule->ResolvedIncludes, " "));

  absl::flat_hash_set<uint32_t> visited;
  auto dfs = [&visited, &makefile](core::models::BuildRule *rule, auto &&dfs) {
    for (const auto &[k, v] : rule->ExportedEnvironmentVars) {
      makefile.Env(
          fmt::format("{}_{}",
                      *(rule->Base->FullQuotedQualifiedNameWithoutVersion), k),
          v);
    }
  };
  dfs(rule, dfs);

  makefile.flush(session->Writer.get());
}

}  // namespace jk::impls::compilers::makefile
