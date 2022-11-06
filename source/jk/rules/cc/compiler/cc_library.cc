// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "fmt/core.h"
#include "fmt/format.h"
#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/rules/cc_binary.hh"
#include "jk/rules/cc/rules/cc_library.hh"
#include "jk/rules/cc/rules/cc_test.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/utils/array.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

core::filesystem::JKProject *IncludesResolvingContextImpl::Project() const {
  return project_;
}

static auto logger = utils::Logger("compiler.cc_library");
std::vector<std::string> GlobalDefines;

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

// makefile {{{
std::string MakefileCCLibraryCompiler::Name() const {
  return "Makefile.cc_library";
}

void MakefileCCLibraryCompiler ::Compile(
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule           = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  // Compile a `cc_library` into three `Unix Makefile`s.
  //   1. flags.make: include all flags to compile all source files, some
  //   variables will be defined:
  //     - WORKING_FOLDER
  //     - CFLAGS
  //     - CXXFLAGS
  //     - CPPFLAGS
  //     - CPP_DEFINES
  //     - CPP_INCLUDES
  //     - DEBUG_CFLAGS
  //     - DEBUG_CXXFLAGS
  //     - RELEASE_CFLAGS
  //     - RELEASE_CXXFLAGS
  //     - PROFILING_CFLAGS
  //     - PROFILING_CXXFLAGS
  //     flags.make -> 3 versions(debug, release, profile)
  //   2. toolchain.make: include compiler settings, some variables will be
  //   defined:
  //     - CXX
  //     - AR
  //     - RM
  //   3. build.make: main build file, include how to all sources files
  GenerateFlags(project,
                wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);

  GenerateToolchain(
      project,
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get(), rule);

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

void MakefileCCLibraryCompiler::AddtionalAction(core::output::UnixMakefile *,
                                                const common::AbsolutePath &,
                                                CCLibrary *) const {
}

// compile database {{{

std::string CompileDatabaseCCLibraryCompiler::Name() const {
  return "CompileDatabase.cc_library";
}

void CompileDatabaseCCLibraryCompiler::Compile(
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule           = _rule->Downcast<CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  auto writer =
      wf->Build(project->ProjectRoot.Sub("compile_commands.json").Stringify());

  std::vector<std::string> c_flags;
  std::vector<std::string> cxx_flags;

  // generate flags {{{
  {
    auto define = rule->ResolveDefinitions();
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(cxx_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
    std::transform(std::begin(define), std::end(define),
                   std::back_inserter(c_flags), [](const std::string &d) {
                     return fmt::format("-D{}", d);
                   });
  }
  {
    IncludesResolvingContextImpl includes_resolving_context(project);
    auto vec = rule->ResolveIncludes(&includes_resolving_context);
    std::transform(std::begin(vec), std::end(vec),
                   std::back_inserter(cxx_flags), [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
    std::transform(std::begin(vec), std::end(vec), std::back_inserter(c_flags),
                   [](const std::string &d) {
                     return fmt::format("-I{}", d);
                   });
  }

  cxx_flags = utils::ConcatArrays(cxx_flags, project->Config().cxxflags,
                                  cppincludes(project), cxxincludes());
  c_flags   = utils::ConcatArrays(c_flags, project->Config().cflags,
                                  cppincludes(project));

  utils::CopyArray(std::begin(rule->CppFlags), std::end(rule->CppFlags),
                   &cxx_flags, &FixCpp20Gcc2ClangFlags);

  utils::CopyArray(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
                   &cxx_flags, &FixCpp20Gcc2ClangFlags);

  std::copy(std::begin(rule->CxxFlags), std::end(rule->CxxFlags),
            std::back_inserter(cxx_flags));

  std::copy(std::begin(rule->CFlags), std::end(rule->CFlags),
            std::back_inserter(c_flags));

  std::copy(std::begin(project->Config().debug_cxxflags_extra),
            std::end(project->Config().debug_cxxflags_extra),
            std::back_inserter(cxx_flags));

  std::copy(std::begin(project->Config().debug_cflags_extra),
            std::end(project->Config().debug_cflags_extra),
            std::back_inserter(c_flags));
  // }}}

  auto sources = rule->ExpandSourceFiles(project, expander);
  std::vector<json> res;
  std::for_each(
      std::begin(sources), std::end(sources),
      [&working_folder, rule, project, &cxx_flags, &c_flags,
       &writer](const std::string &filename) {
        auto sf = SourceFile::Create(rule, rule->Package, filename);

        json res;
        res["file"] =
            project->Resolve(rule->Package->Path).Sub(filename).Stringify();
        std::vector<std::string> command;
        if (sf->IsCppSourceFile()) {
          command.push_back("g++");
          std::copy(std::begin(cxx_flags), std::end(cxx_flags),
                    std::back_inserter(command));
          // FIX g++ option '-fcoroutines' in clang toolchains
          command.push_back("-D__cpp_impl_coroutine");
        } else {
          command.push_back("gcc");
          std::copy(std::begin(c_flags), std::end(c_flags),
                    std::back_inserter(command));
        }
        command.push_back("-o");
        command.push_back(
            sf->FullQualifiedObjectPath(working_folder, "DEBUG").Stringify());
        command.push_back("-c");
        command.push_back(
            project->Resolve(rule->Package->Path).Sub(filename).Stringify());
        res["arguments"] = command;
        res["directory"] = project->ProjectRoot.Stringify();

        writer->WriterJSON(res);
      });

  writer->Flush();
}

// }}}

}  // namespace jk::rules::cc

// vim: fdm=marker
