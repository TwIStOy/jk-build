// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/expander.hh"

#include <glob.h>

#include <cstring>
#include <list>

#include "jk/common/path.hh"
#include "jk/utils/logging.hh"

namespace jk::core::filesystem {

DefaultPatternExpander kDefaultPatternExpander;

static auto logger = utils::Logger("expander");

std::list<std::string> DefaultPatternExpander::Expand(
    const std::string &pattern, const common::AbsolutePath &path) {
  logger->info("Try to expand pattern {} at {}", pattern, path);

  glob_t glob_result;
  memset(&glob_result, 0, sizeof(glob_result));
  auto full_pattern = path.Sub(pattern).Stringify();

  int rv = glob(full_pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
  if (rv != 0) {
    globfree(&glob_result);
    throw JKBuildError("Glob failed with erro code: {}", rv);
  }

  std::list<std::string> result;
  for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
    result.push_back(std::string(glob_result.gl_pathv[i]));
  }
  globfree(&glob_result);

  return result;
}

}  // namespace jk::core::filesystem

// vim: fdm=marker

