// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/compiler/cmake_project.hh"

#include <algorithm>
#include <iterator>
#include <unordered_map>
#include <utility>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/common/flags.hh"
#include "jk/core/builder/custom_command.hh"
#include "jk/core/output/makefile.hh"
#include "jk/rules/external/rules/cmake_project.hh"
#include "jk/utils/str.hh"

namespace jk::rules::external {

std::string MakefileCMakeLibrary::Name() const {
  return "Makefile.cmake_library";
}

void MakefileCMakeLibrary::Compile(
    core::filesystem::ProjectFileSystem *project,
    core::writer::WriterFactory *wf, core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CMakeLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  auto makefile = core::output::NewUnixMakefile("build.make");

  makefile->DefineEnvironment("CMAKE", "cmake");

  makefile->DefineEnvironment("RM", "rm");

  auto [output_folder, indexs, download_target, decompress_target] =
      DownloadAndDecompress(project, makefile.get(), rule, working_folder);
  auto cmake_print_idx = rule->KeyNumber(".cmake");
  indexs.push_back(cmake_print_idx);

  auto build_target = working_folder.Sub("CMAKE_BUILD");
  auto cmake_build_folder = working_folder.Sub("cmake_build_folder");

  {
    auto print_stmt = core::builder::CustomCommandLine::Make({
        "@$(PRINT)",
        "--switch=$(COLOR)",
        "--green",
        "--bold",
        "--progress-num={}"_format(utils::JoinString(",", indexs)),
        "--progress-dir={}"_format(project->BuildRoot),
    });
    auto remove_cmake_cache_stmt = core::builder::CustomCommandLine::Make(
        {"-@$(RM)", cmake_build_folder.Sub("CMakeCache.txt")});

    std::vector<std::string> cmake_stmt{"@$(CMAKE)",
                                        "-S{}"_format(output_folder),
                                        "-B{}"_format(cmake_build_folder)};
    auto variables = rule->CMakeVariable;
    variables.emplace("CMAKE_INSTALL_PREFIX",
                      project->ExternalInstalledPrefix());
    variables.emplace("CMAKE_BUILD_TYPE", "Release");
    variables.emplace("CMAKE_C_FLAGS",
                      "-m{}"_format(ToString(common::FLAGS_platform)));
    variables.emplace("CMAKE_CXX_FLAGS",
                      "-m{}"_format(ToString(common::FLAGS_platform)));
    std::transform(std::begin(variables), std::end(variables),
                   std::back_inserter(cmake_stmt),
                   [](const std::pair<std::string, std::string> &pr) {
                     return "-D{}={}"_format(pr.first, pr.second);
                   });
    auto touch_stmt =
        core::builder::CustomCommandLine::Make({"@touch", build_target});
    auto make_stmt = core::builder::CustomCommandLine::Make(
        {"make", "-f", cmake_build_folder.Sub("Makefile"), "install",
         "-j{}"_format(rule->JobNumber)});

    makefile->AddTarget(
        build_target, {download_target, decompress_target},
        core::builder::CustomCommandLines::Multiple(
            print_stmt, remove_cmake_cache_stmt,
            core::builder::CustomCommandLine::FromVec(cmake_stmt), touch_stmt,
            make_stmt));
  }

  makefile->AddTarget("build", {build_target}, {}, "", true);

  auto w = wf->Build(working_folder.Sub("build.make"));
  makefile->Write(w.get());
}

}  // namespace jk::rules::external

// vim: fdm=marker

