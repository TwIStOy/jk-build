// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <vector>

#include "toml.hpp"

namespace jk::core::filesystem {

struct JKProject;

class Configuration {
 public:
  Configuration(const JKProject *project, const toml::value &value);

  std::string cpplint_path;
  std::string cxx_standard;

  std::vector<std::string> cc;
  std::vector<std::string> cxx;

  std::vector<std::string> compile_flags;
  std::vector<std::string> cflags;
  std::vector<std::string> cxxflags;
  std::vector<std::string> debug_cflags_extra;
  std::vector<std::string> debug_cxxflags_extra;
  std::vector<std::string> release_cflags_extra;
  std::vector<std::string> release_cxxflags_extra;
  std::vector<std::string> profiling_cflags_extra;
  std::vector<std::string> profiling_cxxflags_extra;

  std::vector<std::string> ld_flags;
  std::vector<std::string> release_ld_flags_extra;
  std::vector<std::string> profiling_ld_flags_extra;
  std::vector<std::string> debug_ld_flags_extra;
};

}  // namespace jk::core::filesystem

// vim: fdm=marker
