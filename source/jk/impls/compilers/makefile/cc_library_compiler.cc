// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/cc_library_compiler.hh"

#include <string>

#include "absl/strings/str_join.h"
#include "jk/common/path.hh"
#include "jk/core/generators/makefile.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/rules/cc_library.hh"
#include "range/v3/algorithm/contains.hpp"
#include "range/v3/all.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/concat.hpp"

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

static const auto WORKING_FOLDER  = "WORKING_FOLDER";
static const auto CFLAGS          = "CFLAGS";
static const auto CPPFLAGS        = "CPPFLAGS";
static const auto CXXFLAGS        = "CXXFLAGS";
static const auto CFLAGS_SUFFIX   = "_CFLAGS";
static const auto CPPFLAGS_SUFFIX = "_CPPFLAGS";
static const auto CPP_INCLUDES    = "CPP_INCLUDES";
static const auto CPP_DEFINES     = "CPP_DEFINES";

void generate_flag_file(core::models::Session *session,
                        const common::AbsolutePath &working_folder,
                        rules::CCLibrary *rule) {
  static auto git_desc =
      R"(-DGIT_DESC="\"`cd {} && git describe --tags --always`\"")";
  core::generators::Makefile makefile(working_folder.Sub("flags.make"));

  auto compile_flags = project->Config().compile_flags;
  compile_flags.push_back(fmt::format(git_desc, rule->Package->Path));

  // environments
  makefile.Env(WORKING_FOLDER, working_folder.Stringify());

  makefile.Env(CFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CPPFLAGS, absl::StrJoin(rule->ExpandedCFileFlags, " "));

  makefile.Env(CXXFLAGS,
               absl::StrJoin(ranges::views::concat(
                                 ranges::views::all(rule->CxxFlags),
                                 ranges::views::all(session->ExtraFlags)),
                             " "));

  DEFINE_FLAGS(debug);
  DEFINE_FLAGS(release);
  DEFINE_FLAGS(profiling);

  IncludesResolvingContextImpl includes_resolving_context(project);

  const auto &define = rule->ResolveDefinitions();
  makefile->DefineEnvironment(
      "CPP_DEFINES", utils::JoinString(" ", define.begin(), define.end(),
                                       [](const std::string &inc) {
                                         return fmt::format("-D{}", inc);
                                       }));

  makefile.Env(CPP_DEFINES, absl::StrJoin(rule->Defines, " "));

  makefile.Env(CPP_INCLUDES, absl::StrJoin(rule->ResolvedIncludes, " "));

  {
    std::unordered_set<std::string> record;
    rule->RecursiveExecute(
        [mk = makefile.get(), project](const auto *rule) {
          for (const auto &[k, v] : rule->ExportedEnvironmentVar(project)) {
            mk->DefineEnvironment(
                fmt::format("{}_{}",
                            rule->FullQuotedQualifiedNameWithNoVersion(), k),
                v);
          }
        },
        &record);
  }

  makefile->Write(w);
  return makefile;
}

}  // namespace jk::impls::compilers::makefile
