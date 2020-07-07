// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compile.hh"

#include "jk/core/compile/ir.hh"
#include "jk/lang/cc/rules/cc_library.hh"

namespace jk::lang::cc {

core::compile::ir::Statement CompileIR(
    SourceFile *source, core::filesystem::ProjectFileSystem *project,
    core::compile::ir::IR *ir) {
  core::compile::ir::Statement stmt;

  if (source->IsCppSourceFile()) {
    stmt.Hint =
        "Building CXX Object from {}"_format(source->FullQualifiedPath());
    std::list<core::compile::ir::StmtElement> res{
        ir->Var("toolchain", "CXX"),
        ir->Var("flags", "CXX_DEFINE"),
        ir->Var("flags", "CXX_INCLUDE"),
        ir->Var("flags", "CXX_FLAGS"),
        ir->Var("flags", "CPP_FLAGS"),
        "-o",
        source->FullQualifiedObjectPath(project->BuildRoot).Stringify(),
        "-c",
        project->Resolve(source->FullQualifiedPath()).Stringify()};

    stmt.Elements.insert(stmt.Elements.end(), res.begin(), res.end());
  } else if (source->IsCSourceFile()) {
    stmt.Hint = "Building C Object from {}"_format(source->FullQualifiedPath());
    std::list<core::compile::ir::StmtElement> res{
        ir->Var("toolchain", "CXX"),
        ir->Var("flags", "CXX_DEFINE"),
        ir->Var("flags", "CXX_INCLUDE"),
        ir->Var("flags", "CXX_FLAGS"),
        ir->Var("flags", "C_FLAGS"),
        "-o",
        source->FullQualifiedObjectPath(project->BuildRoot).Stringify(),
        "-c",
        project->Resolve(source->FullQualifiedPath()).Stringify()};

    stmt.Elements.insert(stmt.Elements.end(), res.begin(), res.end());
  } else {
    JK_THROW(core::JKBuildError(
        "unknown file extension: {}",
        source->FullQualifiedPath().Path.extension().string()));
  }

  return stmt;
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
 *     - AR
 *     - RM
 *   - depend.make: source files depends
 */
void CCLibraryCompiler::Compile(
    core::filesystem::ProjectFileSystem *project, core::compile::ir::IR *ir,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<core::rules::CCLibrary>();
  auto working_folder = project->BuildRoot.Sub(
      utils::Replace(rule->FullQualifiedName(), '/', "@"));

  {
    auto &env = ir->Environments["flags"];

    env.NewVar("C_FLAGS", utils::JoinString(" ", rule->CFlags.begin(),
                                            rule->CFlags.end()));
    env.NewVar("CXX_FLAGS", utils::JoinString(" ", rule->CxxFlags.begin(),
                                              rule->CxxFlags.end()));
    env.NewVar("CPP_FLAGS", utils::JoinString(" ", rule->CppFlags.begin(),
                                              rule->CppFlags.end()));
    const auto &define = rule->ResolveDefinitions();
    const auto &include = rule->ResolveIncludes();
    env.NewVar("CXX_DEFINE",
               utils::JoinString(" ", define.begin(), define.end(),
                                 [](const std::string &inc) {
                                   return fmt::format("-D{}", inc);
                                 }));
    env.NewVar("CXX_INCLUDE",
               utils::JoinString(" ", include.begin(), include.end(),
                                 [](const std::string &inc) {
                                   return fmt::format("-I{}", inc);
                                 }));
  }

  {
    auto &env = ir->Environments["toolchain"];
    env.NewVar("CXX", "g++");
    env.NewVar("AR", "ar qc");
    env.NewVar("RM", "rm");
  }

  {
    auto &env = ir->Environments["build"];
    env.NewVar("JK_SOURCE_DIR", project->ProjectRoot.Stringify(),
               "The top-level source directory on which Jk was run.");
    env.NewVar("JK_BINARY_DIR", project->BuildRoot.Stringify(),
               "The top-level build directory on which Jk was run.");
    env.NewVar("PRINT", "jk tools echo_color ");

    auto &build = ir->Pages["build"];
    build.Includes.push_back(core::compile::ir::IncludeItem{
        "flags", "Include the compile flags for this rule's objectes.", true});
    build.Includes.push_back(
        core::compile::ir::IncludeItem{"toolchain", "", true});

    const auto &source_files = rule->ExpandSourceFiles(project, expander);
    std::list<std::string> all_objects;

    for (const auto &source : source_files) {
      auto source_file =
          lang::cc::SourceFile::Create(rule, rule->Package, source);

      auto stmt = CompileIR(source_file, project, ir);

      build.Targets.push_back(core::compile::ir::Target{
          source_file->FullQualifiedObjectPath(working_folder).Stringify(),
          {stmt},
          {source_file->FullQualifiedPath().Stringify(),
           working_folder.Sub("flags.make").Stringify(),
           working_folder.Sub("toolchain.make").Stringify()},
          "",
          false,
          true});

      all_objects.push_back(
          source_file->FullQualifiedObjectPath(working_folder).Stringify());
    }

    {
      auto library_file = working_folder.Sub(rule->ExportedFileName);
      core::compile::ir::Statement stmt;

      stmt.Hint = "Linking CXX static library {}"_format(library_file);

      std::list<core::compile::ir::StmtElement> res{
          ir->Var("toolchain", "AR"), library_file.Stringify(),
          utils::JoinString(" ", all_objects.begin(), all_objects.end())};

      stmt.Elements.insert(stmt.Elements.end(), res.begin(), res.end());

      build.Targets.push_back(core::compile::ir::Target{
          library_file.Stringify(), {stmt}, all_objects, "", false, true});

      auto clean_target = working_folder.Sub("clean").Stringify();
      {
        core::compile::ir::Statement stmt;

        stmt.Hint = "";

        std::list<core::compile::ir::StmtElement> res{
            ir->Var("toolchain", "RM"), library_file.Stringify(),
            utils::JoinString(" ", all_objects.begin(), all_objects.end())};

        stmt.Elements.insert(stmt.Elements.end(), res.begin(), res.end());
        build.Targets.push_back(core::compile::ir::Target{
            clean_target, {stmt}, {}, "", true, false});
      }

      {
        auto build_target = working_folder.Sub("build").Stringify();
        build.Targets.push_back(core::compile::ir::Target{
            build_target,
            {},
            {library_file.Stringify()},
            "Rule to build all files generated by this target.",
            true,
            false});
      }
    }
  }
}

}  // namespace jk::lang::cc

// vim: fdm=marker

