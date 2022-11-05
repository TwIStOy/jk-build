// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/source_file.hh"

#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

auto logger = utils::Logger("source_file");

static std::unordered_set<std::string> CppExtensions = {
    ".cc",
    ".cpp",
    ".cxx",
};
static std::unordered_set<std::string> CExtensions = {
    ".c",
};
static std::unordered_set<std::string> HeaderExtensions = {".h", ".hh", ".hxx",
                                                           ".hpp"};

std::string SourceFile::Stringify() const {
  return "SourceFile({})"_format(FullQualifiedPath());
}

std::unordered_map<std::string, std::unique_ptr<SourceFile>>
    SourceFile::source_files_;

SourceFile *SourceFile::Create(core::rules::BuildRule *rule,
                               core::rules::BuildPackage *package,
                               std::string filename) {
  auto key = package->Path.Sub(filename).Stringify();
  auto it = source_files_.find(key);
  if (it == source_files_.end()) {
    auto x = new SourceFile(rule, package, std::move(filename));
    return x;
  }

  return it->second.get();
}

void SourceFile::ClearCache() {
  source_files_.clear();
}

SourceFile::SourceFile(core::rules::BuildRule *rule,
                       core::rules::BuildPackage *package, std::string filename)
    : Rule(rule), Package(package), FileName(std::move(filename)) {
  if (auto it = source_files_.find(FullQualifiedPath().Stringify());
      it != source_files_.end()) {
    JK_THROW(core::JKBuildError("Source file {} in different rules: {} and {}",
                                FullQualifiedPath(), rule->FullQualifiedName(),
                                it->second->Rule->FullQualifiedName()));
  }

  source_files_[package->Path.Sub(FileName).Stringify()].reset(this);
}

}  // namespace jk::rules::cc
