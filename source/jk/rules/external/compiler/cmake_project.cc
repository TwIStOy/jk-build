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
    core::filesystem::JKProject *project, core::writer::WriterFactory *wf,
    core::rules::BuildRule *_rule,
    core::filesystem::FileNamePatternExpander *expander) const {
  auto rule = _rule->Downcast<CMakeLibrary>();
  auto working_folder = rule->WorkingFolder(project->BuildRoot);

  auto cmake_build_folder = working_folder.Sub("cmake_build_folder");
  std::vector<std::string> cmake_stmt{"@$(CMAKE)", "-S$(THIS_SOURCE_DIR)",
                                      "-B{}"_format(cmake_build_folder)};

  auto variables = rule->CMakeVariable;
  variables.emplace("CMAKE_INSTALL_PREFIX", project->ExternalInstalledPrefix());
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
  CompileImpl(project, wf, rule,
              [](auto makefile) {
                makefile->DefineEnvironment("CMAKE", "cmake");
              },
              {}, cmake_stmt, {}, {});
}

}  // namespace jk::rules::external

// vim: fdm=marker
