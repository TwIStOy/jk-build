// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <list>
#include <unordered_map>
#include <vector>

#include "fmt/core.h"
#include "jk/core/builder/makefile_builder.hh"
#include "jk/core/error.h"
#include "jk/core/rules/package.hh"
#include "jk/lang/cc/cc_library.hh"
#include "jk/lang/cc/source_file.hh"
#include "jk/utils/str.hh"

namespace jk::core::compile {

using fmt::operator"" _a;
using fmt::operator"" _format;

void Compiler::Compile(filesystem::ProjectFileSystem *project,
                       rules::BuildRule *rule) {
  const auto &name = rule->TypeName;
  if (name == "cc_library") {
    Compile_cc_library(project, rule->Downcast<rules::CCLibrary>());
  } else {
    // ...
  }
}

/*
 * A **cc_library** will be compiled into a folder with these files:
 *   - flags.make: includes all flags to compile all source files, three
 *                 VARIABLES will be defined:
 *     - C_FLAGS
 *     - CXX_FLAGS
 *     - CPP_FLAGS
 *     - CXX_DEFINE
 *     - CXX_INCLUDE
 *   - toolchain.make: include compiler settings, VARIABLES:
 *     - CXX
 *   - depend.make: source files depends
 */
void Compiler::Compile_cc_library(  // {{{
    filesystem::ProjectFileSystem *project, rules::CCLibrary *rule) {
  auto working_folder = project->BuildRoot.Sub(
      utils::Replace(rule->FullQualifiedName(), '/', "@"));

  // write "flags.make"
  {
    builder::MakefileBuilder flags;

    flags.DefineEnvironment(
        "C_FLAGS",
        utils::JoinString(" ", rule->CFlags.begin(), rule->CFlags.end()));
    flags.DefineEnvironment(
        "CXX_FLAGS",
        utils::JoinString(" ", rule->CxxFlags.begin(), rule->CxxFlags.end()));
    flags.DefineEnvironment(
        "CPP_FLAGS",
        utils::JoinString(" ", rule->CppFlags.begin(), rule->CppFlags.end()));
    const auto &define = rule->ResolveDefinitions();
    const auto &include = rule->ResolveIncludes();
    flags.DefineEnvironment("CXX_DEFINE",
                            utils::JoinString(" ", define.begin(), define.end(),
                                              [](const std::string &inc) {
                                                return fmt::format("-D{}", inc);
                                              }));
    flags.DefineEnvironment(
        "CXX_INCLUDE", utils::JoinString(" ", include.begin(), include.end(),
                                         [](const std::string &inc) {
                                           return fmt::format("-I{}", inc);
                                         }));
    flags.WriteToFile(working_folder.Sub("flags.make").Path);
  }

  // write "toolchain.make"
  {
    builder::MakefileBuilder toolchain;

    toolchain.DefineEnvironment("CXX", "g++");

    toolchain.WriteToFile(working_folder.Sub("toolchain.make").Path);
  }

  // write "build.make"
  {
    builder::MakefileBuilder build;

    build.DefineCommon();

    build.Include(working_folder.Sub("flags.make").Path,
                  "Include the ccompile flags for this rule's objectes.", true);
    build.Include(working_folder.Sub("toolchain.make").Path, "", true);

    const auto &source_files = rule->ExpandSourceFiles();
    auto idx = 0;
    std::list<std::string> all_objects;

    for (const auto &source : source_files) {
      auto source_file =
          lang::cc::SourceFile::Create(rule, rule->Package, source);

      build.AddTarget(
          source_file->FullQualifiedObjectPath(working_folder).str(),
          {working_folder.Sub("flags.make").str(),
           working_folder.Sub("toolchain.make").str()});

      auto print_stmt =
          "@$(PRINT) --switch=$(COLOR) --green --progress-num={} "
          "--progress-total={} \"Building CXX object {}\""_format(
              idx, source_files.size(),
              source_file->FullQualifiedObjectPath(working_folder).str());

      if (source_file->IsCppSourceFile()) {
        auto build_stmt =
            "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(CPP_FLAGS)"
            " -o {} -c {}"_format(
                source_file->FullQualifiedObjectPath(working_folder).str(),
                source_file->FullQualifiedPath().str());

        build.AddTarget(
            source_file->FullQualifiedObjectPath(working_folder).str(),
            {source_file->FullQualifiedPath().str()}, {print_stmt, build_stmt});
      } else if (source_file->IsCSourceFile()) {
        auto build_stmt =
            "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(C_FLAGS)"
            " -o {} -c {}"_format(
                source_file->FullQualifiedObjectPath(working_folder).str(),
                source_file->FullQualifiedPath().str());

        build.AddTarget(
            source_file->FullQualifiedObjectPath(working_folder).str(),
            {source_file->FullQualifiedPath().str()}, {print_stmt, build_stmt});
      } else {
        throw JKBuildError("unknown file extension: {}",
                           source_file->FullQualifiedPath().Path.extension());
      }

      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder).str());
      idx++;
    }

    auto library_file = working_folder.Sub(rule->ExportedFileName);
    build.AddTarget(
        library_file.str(), all_objects,
        {"@$(PRINT) --switch=$(COLOR) --green --bold --progress-num={} "
         "--progress-total={} \"Linking CXX static library {}\""_format(
             idx, source_files.size(), library_file.str()),
         "$(AR) {} {}"_format(
             library_file.str(),
             utils::JoinString(" ", all_objects.begin(), all_objects.end()))});

    auto clean_target = working_folder.Sub("clean").str();
    build.AddTarget(clean_target, {}, {"$(RM) {}"_format(library_file.str())},
                    "", true);

    auto build_target = working_folder.Sub("build").str();
    build.AddTarget(clean_target, {library_file.str()}, {}, "", true);

    build.WriteToFile(working_folder.Sub("build.make").Path);
  }
}  // }}}

}  // namespace jk::core::compile

// vim: fdm=marker
