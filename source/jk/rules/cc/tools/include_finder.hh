// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/utils/stack.hh"

namespace jk::rules::cc::tools {

//! Try to find all include files recursively
class IncludeFinder {
 public:
  static std::optional<std::string> ParseIncludeLine(std::string_view line);

  void IncludeDirectory(core::filesystem::JKProject *project,
                        const std::string &directory);

  void ParseIncludeDirectoryFlags(core::filesystem::JKProject *project,
                                  const std::string &flag);

  //! Returns all lines include "#include" in file
  std::vector<common::AbsolutePath> Headers(
      core::filesystem::JKProject *project,
      const common::ProjectRelativePath &rp);

  std::vector<common::AbsolutePath> Headers(const common::AbsolutePath &ap);

 private:
  std::optional<common::AbsolutePath> ResolveFileName(
      const std::string &filename);

 private:
  std::vector<common::AbsolutePath> include_directories_;
};

}  // namespace jk::rules::cc::tools

// vim: fdm=marker
