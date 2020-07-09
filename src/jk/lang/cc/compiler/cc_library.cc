// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_binary.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/lang/cc/rules/cc_test.hh"
#include "jk/lang/cc/source_file.hh"

namespace jk::lang::cc {

std::string MakefileCCLibraryCompiler::Name() const {
  return "Makefile.cc_library";
}

void MakefileCCLibraryCompiler ::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<core::rules::CCLibrary>();
  auto working_folder = project->BuildRoot.Sub(
      utils::Replace(rule->FullQualifiedName(), '/', "@"));
  // Compile a `cc_library` into three `Unix Makefile`s.
  //   1. flags.make: include all flags to compile all source files, some
  //   variables will be defined:
  //     - C_FLAGS
  //     - CXX_FLAGS
  //     - CPP_FLAGS
  //     - CXX_DEFINE
  //     - CXX_INCLUDE
  //   2. toolchain.make: include compiler settings, some variables will be
  //   defined:
  //     - CXX
  //     - AR
  //     - RM
  //   3. build.make: main build file, include how to all sources files
  GenerateFlags(wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);
  GenerateToolchain(
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get());

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

void MakefileCCLibraryCompiler::GenerateBuild(
    core::filesystem::ProjectFileSystem *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    core::rules::CCLibrary *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  core::output::UnixMakefile build("build.make");

  build.DefineCommon(project);

  build.Include(working_folder.Sub("flags.make").Path,
                "Include the compile flags for this rule's objectes.", true);
  build.Include(working_folder.Sub("toolchain.make").Path, "", true);

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  // headers
  std::list<std::string> all_dep_headers;
  for (auto dep : rule->DependenciesInOrder()) {
    auto vec = dep->ExportedHeaders();
    std::transform(
        vec.begin(), vec.end(), std::back_inserter(all_dep_headers),
        [project](const std::string &filename) {
          return project
              ->Resolve(common::ProjectRelativePath{fs::path{filename}})
              .Stringify();
        });
  }

  auto idx = 0;
  std::list<std::string> all_objects;
  for (const auto &filename : source_files) {
    auto source_file =
        lang::cc::SourceFile::Create(rule, rule->Package, filename);

    MakeSourceFile(project, source_file, idx, source_files.size(),
                   all_dep_headers, &build, working_folder);
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
  build.AddTarget(build_target, {library_file.Stringify()}, {},
                  "Rule to build all files generated by this target.", true);

  build.Write(w);
}

void MakefileCCLibraryCompiler::MakeSourceFile(
    core::filesystem::ProjectFileSystem *project, SourceFile *source_file,
    uint32_t idx, uint32_t source_files_count,
    const std::list<std::string> &headers, core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  build->AddTarget(
      source_file->FullQualifiedObjectPath(working_folder).Stringify(),
      {working_folder.Sub("flags.make").Stringify(),
       working_folder.Sub("toolchain.make").Stringify()});

  auto print_stmt =
      "@$(PRINT) --switch=$(COLOR) --green --progress-num={} "
      "--progress-total={} \"Building CXX object {}\""_format(
          idx, source_files_count,
          source_file->FullQualifiedObjectPath(working_folder).Stringify());

  auto dep = headers;
  dep.push_back(source_file->FullQualifiedPath().Stringify());
  if (source_file->IsCppSourceFile()) {
    auto build_stmt =
        "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(CPP_FLAGS)"
        " -o {} -c {}"_format(
            source_file->FullQualifiedObjectPath(working_folder).Stringify(),
            project->Resolve(source_file->FullQualifiedPath()).Stringify());

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder).Stringify(), dep,
        {print_stmt, build_stmt});
  } else if (source_file->IsCSourceFile()) {
    auto build_stmt =
        "$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(C_FLAGS)"
        " -o {} -c {}"_format(
            source_file->FullQualifiedObjectPath(working_folder).Stringify(),
            project->Resolve(source_file->FullQualifiedPath()).Stringify());

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder).Stringify(), dep,
        {print_stmt, build_stmt});
  } else {
    JK_THROW(core::JKBuildError(
        "unknown file extension: {}",
        source_file->FullQualifiedPath().Path.extension().string()));
  }
}

void MakefileCCLibraryCompiler::GenerateToolchain(
    core::writer::Writer *w) const {
  core::output::UnixMakefile makefile("toolchain.make");

  makefile.DefineEnvironment("CXX", "g++");
  makefile.DefineEnvironment("AR", "ar");
  makefile.DefineEnvironment("RM", "rm", "The command to remove a file.");

  makefile.Write(w);
}

void MakefileCCLibraryCompiler::GenerateFlags(
    core::writer::Writer *w, core::rules::CCLibrary *rule) const {
  core::output::UnixMakefile makefile("flags.make");

  makefile.DefineEnvironment(
      "C_FLAGS",
      utils::JoinString(" ", rule->CFlags.begin(), rule->CFlags.end()));
  makefile.DefineEnvironment(
      "CXX_FLAGS",
      utils::JoinString(" ", rule->CxxFlags.begin(), rule->CxxFlags.end()));
  makefile.DefineEnvironment(
      "CPP_FLAGS",
      utils::JoinString(" ", rule->CppFlags.begin(), rule->CppFlags.end()));
  const auto &define = rule->ResolveDefinitions();
  const auto &include = rule->ResolveIncludes();
  makefile.DefineEnvironment(
      "CXX_DEFINE", utils::JoinString(" ", define.begin(), define.end(),
                                      [](const std::string &inc) {
                                        return fmt::format("-D{}", inc);
                                      }));
  makefile.DefineEnvironment(
      "CXX_INCLUDE", utils::JoinString(" ", include.begin(), include.end(),
                                       [](const std::string &inc) {
                                         return fmt::format("-I{}", inc);
                                       }));

  makefile.Write(w);
}

}  // namespace jk::lang::cc

// vim: fdm=marker
