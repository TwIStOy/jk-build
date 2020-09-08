// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/proto_library.hh"

#include <algorithm>
#include <memory>
#include <utility>

#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/output/makefile.hh"
#include "jk/rules/cc/rules/proto_library.hh"

namespace jk::rules::cc {

std::string MakefileProtoLibraryCompiler::Name() const {
  return "Makefile.proto_library";
}

void MakefileProtoLibraryCompiler::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<ProtoLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  GenerateFlags(wf->Build(working_folder.Sub("flags.make").Stringify()).get(),
                rule);

  GenerateToolchain(
      wf->Build(working_folder.Sub("toolchain.make").Stringify()).get(), rule);

  GenerateBuild(project, working_folder,
                wf->Build(working_folder.Sub("build.make").Stringify()).get(),
                rule, expander);
}

core::output::UnixMakefilePtr MakefileProtoLibraryCompiler::GenerateBuild(
    core::filesystem::ProjectFileSystem *project,
    const common::AbsolutePath &working_folder, core::writer::Writer *w,
    ProtoLibrary *rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto build = std::make_unique<core::output::UnixMakefile>("build.make");

  build->DefineCommon(project);

  build->Include(working_folder.Sub("flags.make").Path,
                 "Include the compile flags for this rule's objectes.", true);
  build->Include(working_folder.Sub("toolchain.make").Path, "", true);

  build->AddTarget("all", {"DEBUG"}, {}, "", true);

  build->AddTarget("jk_force", {}, {}, "This target is always outdated.", true);

  const auto &source_files = rule->ExpandSourceFiles(project, expander);

  auto counter = common::Counter();
  std::list<uint32_t> progress_num;

  auto clean_target = working_folder.Sub("clean").Stringify();
  core::builder::CustomCommandLines clean_statements;

  auto library_progress_num = counter->Next();
  progress_num.push_back(library_progress_num);

  // genereate ".cc" and ".h" files
  for (const auto &filename : source_files) {
    auto source_file = SourceFile::Create(rule, rule->Package, filename);
    auto num = counter->Next();
    progress_num.push_back(num);
    auto print_stmt = core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green",
         "--progress-num={}"_format(num),
         "--progress-dir={}"_format(project->BuildRoot),
         "Compiling proto file into .cc/.h {}"_format(
             source_file->FullQualifiedPath())});
    auto protoc = core::builder::CustomCommandLine::Make(
        {"${third_party_protobuf_PROTOC}",
         "--python_out={}"_format(working_folder.Sub("pb")),
         "--cpp_out={}"_format(working_folder.Sub("pb")),
         "-I{}"_format(project->ProjectRoot),
         project->Resolve(source_file->FullQualifiedPath())});

    build->AddTarget(
        "{}.cc {}.h"_format(source_file->FullQualifiedPbPath(working_folder),
                            source_file->FullQualifiedPbPath(working_folder)),
        {project->Resolve(source_file->FullQualifiedPath())}),
        core::builder::CustomCommandLines::Multiple(print_stmt, protoc);

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"$(RM)",
         "{}.cc"_format(source_file->FullQualifiedPbPath(working_folder))}));
    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"$(RM)",
         "{}.h"_format(source_file->FullQualifiedPbPath(working_folder))}));
  }

  for (const auto &build_type : common::FLAGS_BuildTypes) {
    std::list<std::string> all_objects;

    for (const auto &filename : source_files) {
      auto cc_filename =
          filename.substr(0, filename.find_last_of('.')) + ".pb.cc";
      auto source_file = SourceFile::Create(rule, rule->Package, cc_filename);

      MakeSourceFile(project, build_type, source_file, {}, build.get(),
                     working_folder);

      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder, build_type));
    }

    auto library_file =
        working_folder.Sub(build_type).Sub(rule->ExportedFileName);
    build->AddTarget(library_file.Stringify(), {"jk_force"});
    auto ar_stmt = core::builder::CustomCommandLine::Make({
        "@$(AR)",
        library_file.Stringify(),
    });
    std::copy(std::begin(all_objects), std::end(all_objects),
              std::back_inserter(ar_stmt));

    build->AddTarget(
        library_file.Stringify(), all_objects,
        core::builder::CustomCommandLines::Multiple(
            core::builder::CustomCommandLine::Make(
                {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
                 "--progress-num={}"_format(
                     utils::JoinString(",", progress_num)),
                 "--progress-dir={}"_format(project->BuildRoot),
                 "Linking CXX static library {}"_format(
                     library_file.Stringify())}),
            core::builder::CustomCommandLine::Make(
                {"@$(MKDIR)", library_file.Path.parent_path().string()}),
            ar_stmt));

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"$(RM)", library_file.Stringify()}));
    for (const auto &it : all_objects) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"$(RM)", it}));
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

}  // namespace jk::rules::cc

// vim: fdm=marker

