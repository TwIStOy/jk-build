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
                       writer::WriterFactory *factory, rules::BuildRule *rule) {
  const auto &name = rule->TypeName;
  if (name == "cc_library") {
    Compile_cc_library(project, factory, rule->Downcast<rules::CCLibrary>());
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
    filesystem::ProjectFileSystem *project, writer::WriterFactory *factory,
    rules::CCLibrary *rule) {
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
    flags.WriteTo(
        factory->Build(working_folder.Sub("flags.make").Stringify()).get());
  }

  // write "toolchain.make"
  {
    builder::MakefileBuilder toolchain;

    toolchain.DefineEnvironment("CXX", "g++");

    toolchain.WriteTo(
        factory->Build(working_folder.Sub("toolchain.make").Stringify()).get());
  }

  // write "build.make"
  {
    builder::MakefileBuilder build;

    build.DefineCommon();

    build.Include(working_folder.Sub("flags.make").Path,
                  "Include the ccompile flags for this rule's objectes.", true);
    build.Include(working_folder.Sub("toolchain.make").Path, "", true);

    const auto &source_files = rule->ExpandSourceFiles(project, Expander);
    auto idx = 0;
    std::list<std::string> all_objects;

    for (const auto &source : source_files) {
      auto source_file =
          lang::cc::SourceFile::Create(rule, rule->Package, source);

      build.AddTarget(
          source_file->FullQualifiedObjectPath(working_folder).Stringify(),
          {working_folder.Sub("flags.make").Stringify(),
           working_folder.Sub("toolchain.make").Stringify()});

      auto print_stmt =
          "@$(PRINT) --switch=$(COLOR) --green --progress-num={} "
          "--progress-total={} \"Building CXX object {}\""_format(
              idx, source_files.size(),
              source_file->FullQualifiedObjectPath(working_folder).Stringify());

      if (source_file->IsCppSourceFile()) {
        auto build_stmt =
            "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(CPP_FLAGS)"
            " -o {} -c {}"_format(
                source_file->FullQualifiedObjectPath(working_folder)
                    .Stringify(),
                source_file->FullQualifiedPath().Stringify());

        build.AddTarget(
            source_file->FullQualifiedObjectPath(working_folder).Stringify(),
            {source_file->FullQualifiedPath().Stringify()},
            {print_stmt, build_stmt});
      } else if (source_file->IsCSourceFile()) {
        auto build_stmt =
            "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(C_FLAGS)"
            " -o {} -c {}"_format(
                source_file->FullQualifiedObjectPath(working_folder)
                    .Stringify(),
                source_file->FullQualifiedPath().Stringify());

        build.AddTarget(
            source_file->FullQualifiedObjectPath(working_folder).Stringify(),
            {source_file->FullQualifiedPath().Stringify()},
            {print_stmt, build_stmt});
      } else {
        throw JKBuildError(
            "unknown file extension: {}",
            source_file->FullQualifiedPath().Path.extension().string());
      }

      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder).Stringify());
      idx++;
    }

    auto library_file = working_folder.Sub(rule->ExportedFileName);
    build.AddTarget(
        library_file.Stringify(), all_objects,
        {"@$(PRINT) --switch=$(COLOR) --green --bold --progress-num={} "
         "--progress-total={} \"Linking CXX static library {}\""_format(
             idx, source_files.size(), library_file.Stringify()),
         "$(AR) {} {}"_format(
             library_file.Stringify(),
             utils::JoinString(" ", all_objects.begin(), all_objects.end()))});

    auto clean_target = working_folder.Sub("clean").Stringify();
    build.AddTarget(clean_target, {},
                    {"$(RM) {}"_format(library_file.Stringify())}, "", true);

    auto build_target = working_folder.Sub("build").Stringify();
    build.AddTarget(clean_target, {library_file.Stringify()}, {}, "", true);

    build.WriteTo(
        factory->Build(working_folder.Sub("build.make").Stringify()).get());
  }
}  // }}}

}  // namespace jk::core::compile

// vim: fdm=marker
