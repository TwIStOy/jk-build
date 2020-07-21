// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/source_file.hh"

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk {
namespace lang {
namespace cc {

static std::unordered_set<std::string> CppExtensions = {
    ".cc", ".cpp", ".cxx", ".CC", ".CPP", ".CXX",
};
static std::unordered_set<std::string> CExtensions = {
    ".c",
};

std::string SourceFile::Stringify() const {
  return "SourceFile({})"_format(FullQualifiedPath());
}
std::unordered_map<std::string, std::unique_ptr<SourceFile>>
    SourceFile::source_files_;

SourceFile *SourceFile::Create(core::rules::BuildRule *rule,
                               core::rules::BuildPackage *package,
                               std::string filename) {
  auto it = source_files_.find(package->Path.Sub(filename).Stringify());
  if (it == source_files_.end()) {
    auto x = new SourceFile(rule, package, std::move(filename));
    return x;
  }

  return it->second.get();
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
  source_files_[FileName].reset(this);
}

common::ProjectRelativePath SourceFile::FullQualifiedPath() const {
  return Package->Path.Sub(FileName);
}

common::AbsolutePath SourceFile::FullQualifiedObjectPath(
    const common::AbsolutePath &new_root, const std::string &build_type) const {
  auto p = new_root.Path / build_type / Package->Path.Sub(FileName).Path;
  p.replace_filename(p.filename().string() + ".o");
  return common::AbsolutePath{p};
}

common::AbsolutePath SourceFile::FullQualifiedLintPath(
    const common::AbsolutePath &new_root) const {
  auto p = new_root.Path / Package->Path.Sub(FileName).Path;
  p.replace_filename(p.filename().string() + ".lint");
  return common::AbsolutePath{p};
}

common::ProjectRelativePath SourceFile::FullQualifiedObjectPath() const {
  auto p = Package->Path.Sub(FileName);
  p.Path.replace_filename(p.Path.filename().string() + ".o");
  return p;
}

bool SourceFile::IsCSourceFile() const {
  auto p = fs::path(Package->Name) / FileName;
  return CExtensions.find(p.extension()) != CExtensions.end();
}

bool SourceFile::IsCppSourceFile() const {
  auto p = fs::path(Package->Name) / FileName;
  return CppExtensions.find(p.extension()) != CppExtensions.end();
}

bool SourceFile::IsSourceFile() const {
  return IsCSourceFile() || IsCppSourceFile();
}

}  // namespace cc
}  // namespace lang
}  // namespace jk

