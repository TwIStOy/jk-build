// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_binary.hh"

#include <string>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/core/rules/package.hh"
#include "jk/lang/cc/rules/cc_library_helper.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"

namespace jk::lang::cc {

std::string MakefileCCBinaryCompiler::Name() const {
  return "Makefile.cc_binary";
}

void MakefileCCBinaryCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<core::rules::CCBinary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  GenerateFlags(wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);
  GenerateToolchain(
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get());

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

static std::vector<std::string> LDFLAGS = {
    "-L.build/.lib/m${PLATFORM}/lib",
    "-m${PLATFORM}",
    "-levent",
    "-levent_pthreads",
    "-pthread",
    "-lpthread",
    "-Wl,--no-as-needed",
    "-ldl",
    "-lrt",
};

static std::vector<std::string> RELEASE_LDFLAGS = {
    "${LDFLAGS}",
    "-Wl,-Bstatic,-ltcmalloc_minimal",
    "-Wl,-Bdynamic",
};

static std::vector<std::string> PROFILING_LDFLAGS = {
    "${LDFLAGS}",
    "-Wl,--whole-archive",
    "-Wl,-Bstatic,-ltcmalloc_and_profiler",
    "-Wl,--no-whole-archive",
    "-Wl,-Bdynamic",
};

static std::vector<std::string> DEBUG_LDFLAGS_BEFORE = {
    "-ftest-coverage",
    "-fprofile-arcs",
};

static std::vector<std::string> DEBUG_LDFLAGS_AFTER = {
    "-static-libasan",
    "-Wl,-Bstatic,-lasan",
    "-Wl,-Bdynamic",
    "-ldl",
};

core::output::UnixMakefilePtr MakefileCCBinaryCompiler::GenerateBuild(
    core::filesystem::ProjectFileSystem *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    core::rules::CCLibrary *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<core::rules::CCBinary>();
  core::output::UnixMakefilePtr build(
      new core::output::UnixMakefile{"build.make"});

  build->DefineCommon(project);

  build->Include(working_folder.Sub("flags.make").Path,
                 "Include the compile flags for this rule's objectes.", true);
  build->Include(working_folder.Sub("toolchain.make").Path, "", true);

  build->DefineEnvironment(
      "DEBUG_LDFLAGS",
      utils::JoinString(" ", utils::ConcatArrays(DEBUG_LDFLAGS_BEFORE, LDFLAGS,
                                                 DEBUG_LDFLAGS_AFTER)));
  build->DefineEnvironment(
      "RELEASE_LDFLAGS",
      utils::JoinString(" ", utils::ConcatArrays(LDFLAGS, RELEASE_LDFLAGS)));

  build->DefineEnvironment(
      "PROFILING_LDFLAGS",
      utils::JoinString(" ", utils::ConcatArrays(LDFLAGS, PROFILING_LDFLAGS)));

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  // headers
  std::list<std::string> all_dep_headers = MergeDepHeaders(rule, project);
  std::list<uint32_t> progress_num;

  auto counter = common::Counter();

  // lint files first
  for (const auto &filename : source_files) {
    auto source_file =
        lang::cc::SourceFile::Create(rule, rule->Package, filename);

    progress_num.push_back(
        LintSourceFile(project, source_file, build.get(), working_folder));
    progress_num.push_back(source_file->ProgressNum);
  }

  auto clean_target = working_folder.Sub("clean").Stringify();
  std::list<std::string> clean_statements;

  auto binary_progress_num = counter->Next();
  progress_num.push_back(binary_progress_num);
  for (const auto &build_type : BuildTypes) {
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
    auto binary_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    auto deps_and_flags =
        rule->ResolveDependenciesAndLdFlags(project->BuildRoot, build_type);

    auto binary_deps = all_objects;
    for (auto dep : rule->Dependencies) {
      auto names = dep->ExportedFilesSimpleName();
      auto dep_working_folder = dep->WorkingFolder(project->BuildRoot);
      for (const auto &name : names) {
        binary_deps.push_back(
            dep_working_folder.Sub(build_type).Sub(name).Stringify());
      }
    }
    build->AddTarget(
        binary_file.Stringify(), binary_deps,
        {"@$(PRINT) --switch=$(COLOR) --green --bold --progress-num={} "
         "--progress-dir={} \"Linking binary {}\""_format(
             utils::JoinString(",", progress_num), project->BuildRoot,
             binary_file.Stringify()),
         "$(LINKER) {} {} -g ${}{}_LDFLAGS{} -o {} "_format(
             utils::JoinString(" ", all_objects.begin(), all_objects.end()),
             utils::JoinString(" ", deps_and_flags.begin(),
                               deps_and_flags.end()),
             "{", build_type, "}", binary_file.Stringify())});
    clean_statements.push_back("$(RM) {}"_format(binary_file.Stringify()));
    for (const auto &obj : all_objects) {
      clean_statements.push_back("$(RM) {}"_format(obj));
    }

    auto build_target = working_folder.Sub(build_type).Sub("build").Stringify();
    build->AddTarget(build_target, {binary_file.Stringify()}, {},
                     "Rule to build all files generated by this target.", true);

    build->AddTarget(build_type, {build_target}, {}, "", true);
  }

  build->AddTarget(clean_target, {}, clean_statements, "", true);

  build->Write(w);

  return build;
}

}  // namespace jk::lang::cc

// vim: fdm=marker

