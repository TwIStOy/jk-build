// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_library.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/core/compile/compile.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/output/makefile.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_binary.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/lang/cc/rules/cc_library_helper.hh"
#include "jk/lang/cc/rules/cc_test.hh"
#include "jk/lang/cc/source_file.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"

namespace jk::lang::cc {

std::string MakefileCCLibraryCompiler::Name() const {
  return "Makefile.cc_library";
}

void MakefileCCLibraryCompiler ::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<core::rules::CCLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);
  // Compile a `cc_library` into three `Unix Makefile`s.
  //   1. flags.make: include all flags to compile all source files, some
  //   variables will be defined:
  //     - C_FLAGS
  //     - CXX_FLAGS
  //     - CPP_FLAGS
  //     - CXX_DEFINE
  //     - CXX_INCLUDE
  //     flags.make -> 3 versions(debug, release, profile)
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

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateBuild(
    core::filesystem::ProjectFileSystem *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    core::rules::CCLibrary *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto build = std::make_unique<core::output::UnixMakefile>("build.make");

  build->DefineCommon(project);

  build->Include(working_folder.Sub("flags.make").Path,
                 "Include the compile flags for this rule's objectes.", true);
  build->Include(working_folder.Sub("toolchain.make").Path, "", true);

  build->AddTarget("all", {"DEBUG"}, {}, "", true);

  build->AddTarget("jk_force", {}, {}, "This target is always oudated.", true);

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  auto counter = common::Counter();

  // headers
  std::list<std::string> all_dep_headers = MergeDepHeaders(rule, project);
  std::list<uint32_t> progress_num;

  // lint sources first
  for (const auto &filename : source_files) {
    auto source_file =
        lang::cc::SourceFile::Create(rule, rule->Package, filename);

    progress_num.push_back(
        LintSourceFile(project, source_file, build.get(), working_folder));
    progress_num.push_back(source_file->ProgressNum);
  }

  auto clean_target = working_folder.Sub("clean").Stringify();
  std::list<std::string> clean_statements;

  auto library_progress_num = counter->Next();
  progress_num.push_back(library_progress_num);
  for (const auto &build_type : common::FLAGS_BuildTypes) {
    std::list<std::string> all_objects;

    for (const auto &filename : source_files) {
      auto source_file =
          lang::cc::SourceFile::Create(rule, rule->Package, filename);

      MakeSourceFile(project, build_type, source_file, all_dep_headers,
                     build.get(), working_folder);
      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder, build_type)
              .Stringify());
    }

    auto library_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    build->AddTarget(library_file.Stringify(), {"jk_force"});
    build->AddTarget(
        library_file.Stringify(), all_objects,
        {"@$(PRINT) --switch=$(COLOR) --green --bold --progress-num={} "
         "--progress-dir={} \"Linking CXX static library {}\""_format(
             utils::JoinString(",", progress_num), project->BuildRoot,
             library_file.Stringify()),
         "@$(MKDIR) {}"_format(library_file.Path.parent_path().string()),
         "@$(AR) {} {}"_format(
             library_file.Stringify(),
             utils::JoinString(" ", all_objects.begin(), all_objects.end()))});
    clean_statements.push_back("$(RM) {}"_format(library_file.Stringify()));
    for (const auto &it : all_objects) {
      clean_statements.push_back("$(RM) {}"_format(it));
    }

    auto build_target = working_folder.Sub(build_type).Sub("build").Stringify();
    build->AddTarget(build_target, {library_file.Stringify()}, {},
                     "Rule to build all files generated by this target.", true);

    build->AddTarget(build_type, {build_target}, {}, "", true);
  }

  build->AddTarget(clean_target, {}, std::move(clean_statements), "", true);

  build->Write(w);

  return build;
}

uint32_t MakefileCCLibraryCompiler::LintSourceFile(
    core::filesystem::ProjectFileSystem *project, SourceFile *source_file,
    core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  auto progress_num = common::Counter()->Next();

  auto print_stmt =
      "@$(PRINT) --switch=$(COLOR) --green --progress-num={} "
      "--progress-dir={} \"Linting CXX file {}\""_format(
          progress_num, project->BuildRoot,
          project->Resolve(source_file->FullQualifiedPath()));
  auto lint_stmt = "@$(CPPLINT) {} >/dev/null"_format(
      project->Resolve(source_file->FullQualifiedPath()));
  auto mkdir_stmt =
      "@$(MKDIR) {}"_format(source_file->FullQualifiedLintPath(working_folder)
                                .Path.parent_path()
                                .string());
  auto touch_stmt = "@touch {}"_format(
      source_file->FullQualifiedLintPath(working_folder).Stringify());

  build->AddTarget(
      source_file->FullQualifiedLintPath(working_folder).Stringify(),
      {
          project->Resolve(source_file->FullQualifiedPath()).Stringify(),
          working_folder.Sub("toolchain.make").Stringify(),
      },
      {print_stmt, lint_stmt, mkdir_stmt, touch_stmt});
  return progress_num;
}

void MakefileCCLibraryCompiler::MakeSourceFile(
    core::filesystem::ProjectFileSystem *project, const std::string &build_type,
    SourceFile *source_file, const std::list<std::string> &headers,
    core::output::UnixMakefile *build,
    const common::AbsolutePath &working_folder) const {
  build->AddTarget(
      source_file->FullQualifiedObjectPath(working_folder, build_type)
          .Stringify(),
      {working_folder.Sub("flags.make").Stringify(),
       working_folder.Sub("toolchain.make").Stringify(),
       source_file->FullQualifiedLintPath(working_folder).Stringify()});

  auto print_stmt =
      "@$(PRINT) --switch=$(COLOR) --green --progress-num={} "
      "--progress-dir={} \"Building CXX object {}\""_format(
          source_file->ProgressNum, project->BuildRoot,
          source_file->FullQualifiedObjectPath(working_folder, build_type)
              .Stringify());

  auto dep = headers;
  dep.push_back(project->Resolve(source_file->FullQualifiedPath()).Stringify());
  auto mkdir_stmt = "@$(MKDIR) {}"_format(
      source_file->FullQualifiedObjectPath(working_folder, build_type)
          .Path.parent_path()
          .string());

  if (source_file->IsCppSourceFile()) {
    auto build_stmt =
        "@$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(CPP_FLAGS) "
        "$({}_CPP_FLAGS)"
        " -o {} -c {}"_format(
            build_type,
            source_file->FullQualifiedObjectPath(working_folder, build_type)
                .Stringify(),
            project->Resolve(source_file->FullQualifiedPath()).Stringify());

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder, build_type)
            .Stringify(),
        dep, {print_stmt, mkdir_stmt, build_stmt});
  } else if (source_file->IsCSourceFile()) {
    auto build_stmt =
        "@$(CXX) $(CXX_DEFINE) $(CXX_INCLUDE) $(CXX_FLAGS) $(C_FLAGS) "
        "$({}_C_FLAGS)"
        " -o {} -c {}"_format(
            build_type,
            source_file->FullQualifiedObjectPath(working_folder, build_type)
                .Stringify(),
            project->Resolve(source_file->FullQualifiedPath()).Stringify());

    build->AddTarget(
        source_file->FullQualifiedObjectPath(working_folder, build_type)
            .Stringify(),
        dep, {print_stmt, mkdir_stmt, build_stmt});
  } else {
    JK_THROW(core::JKBuildError(
        "unknown file extension: {}",
        source_file->FullQualifiedPath().Path.extension().string()));
  }
}

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateToolchain(
    core::writer::Writer *w) const {
  auto makefile =
      std::make_unique<core::output::UnixMakefile>("toolchain.make");

  if (common::FLAGS_platform == common::Platform::k64) {
    makefile->DefineEnvironment("CXX", "g++ -m64");
  } else {
    makefile->DefineEnvironment("CXX", "g++ -m32");
  }

  makefile->DefineEnvironment("LINKER", "g++");

  makefile->DefineEnvironment("AR", "ar rcs");

  makefile->DefineEnvironment("RM", "rm", "The command to remove a file.");

  makefile->DefineEnvironment("CPPLINT", "cpplint");

  makefile->DefineEnvironment("MKDIR", "mkdir -p");

  makefile->Write(w);
  return makefile;
}

// {{{
static std::vector<std::string> COMPILE_FLAGS = {
    "-MMD",
    "-msse3",
    "-fPIC",
    "-fstrict-aliasing",
    "-Wall",
    "-Wextra",
    "-Wtrigraphs",
    "-Wuninitialized",
    "-Wwrite-strings",
    "-Wpointer-arith",
    "-Wredundant-decls",
    "-Wunused",
    "-Wmissing-include-dirs",
    "-Wno-missing-field-initializers",
    "-Werror",
};

static std::vector<std::string> CFLAGS = {
    "-D_GNU_SOURCE", "-Werror-implicit-function-declaration"};

static std::vector<std::string> CPPFLAGS = {"-std=c++11",
                                            "-Wvla",
                                            "-Wnon-virtual-dtor",
                                            "-Woverloaded-virtual",
                                            "-Wno-invalid-offsetof",
                                            "-Werror=non-virtual-dtor",
                                            "-D__STDC_FORMAT_MACROS",
                                            "-DUSE_SYMBOLIZE",
                                            "-I.",
                                            "-isystem",
                                            ".build/.lib/m${PLATFORM}/include",
                                            "-I.build/include",
                                            "-I.build/pb/c++"};

static std::vector<std::string> DEBUG_CFLAGS_EXTRA = {
    "-O0",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fsanitize=address",
    "-fno-inline",
    "-fno-omit-frame-pointer",
    "-fno-builtin",
    "-fno-optimize-sibling-calls",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> DEBUG_CPPFLAGS_EXTRA = {
    "-O0",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fsanitize=address",
    "-fno-inline",
    "-fno-omit-frame-pointer",
    "-fno-builtin",
    "-fno-optimize-sibling-calls",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
    "-ftest-coverage",
    "-fprofile-arcs"};

static std::vector<std::string> RELEASE_CFLAGS_EXTRA = {
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free3",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> RELEASE_CPPFLAGS_EXTRA = {
    "-DNDEBUG",
    "-DUSE_TCMALLOC=1",
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> PROFILING_CFLAGS_EXTRA = {
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free3",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};

static std::vector<std::string> PROFILING_CPPFLAGS_EXTRA = {
    "-DNDEBUG",
    "-DUSE_TCMALLOC=1",
    "-DHEAP_PROFILING",
    "-DNDEBUG",
    "-O3",
    "-ggdb3",
    "-Wformat=2",
    "-Wstrict-aliasing",
    "-fno-builtin-malloc",
    "-fno-builtin-calloc",
    "-fno-builtin-realloc",
    "-fno-builtin-free",
    "-Wframe-larger-than=65535",
    "-fno-omit-frame-pointer",
};
// }}}

#define DEFINE_FLAGS(tag)                                                 \
  makefile->DefineEnvironment(                                            \
      #tag "_C_FLAGS",                                                    \
      utils::JoinString(" ", utils::ConcatArrays(COMPILE_FLAGS, CFLAGS,   \
                                                 tag##_CFLAGS_EXTRA)));   \
                                                                          \
  makefile->DefineEnvironment(                                            \
      #tag "_CPP_FLAGS",                                                  \
      utils::JoinString(" ", utils::ConcatArrays(COMPILE_FLAGS, CPPFLAGS, \
                                                 tag##_CPPFLAGS_EXTRA)));

core::output::UnixMakefilePtr MakefileCCLibraryCompiler::GenerateFlags(
    core::writer::Writer *w, core::rules::CCLibrary *rule) const {
  auto makefile = std::make_unique<core::output::UnixMakefile>("flags.make");

  makefile->DefineEnvironment(
      "C_FLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CFlags)));

  makefile->DefineEnvironment(
      "CPP_FLAGS", utils::JoinString(" ", utils::ConcatArrays(rule->CppFlags)));

  DEFINE_FLAGS(DEBUG);
  DEFINE_FLAGS(RELEASE);
  DEFINE_FLAGS(PROFILING);

  makefile->DefineEnvironment(
      "CXX_FLAGS",
      utils::JoinString(" ", rule->CxxFlags.begin(), rule->CxxFlags.end()));

  const auto &define = rule->ResolveDefinitions();
  const auto &include = rule->ResolveIncludes();
  makefile->DefineEnvironment(
      "CXX_DEFINE", utils::JoinString(" ", define.begin(), define.end(),
                                      [](const std::string &inc) {
                                        return fmt::format("-D{}", inc);
                                      }));
  makefile->DefineEnvironment(
      "CXX_INCLUDE", utils::JoinString(" ", include.begin(), include.end(),
                                       [](const std::string &inc) {
                                         return fmt::format("-I{}", inc);
                                       }));

  makefile->Write(w);
  return makefile;
}

}  // namespace jk::lang::cc

// vim: fdm=marker
