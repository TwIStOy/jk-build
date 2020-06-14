// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/source_file.hh"

#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"

namespace jk {
namespace lang {
namespace cc {

std::string SourceFile::FullQualifiedName() const {
  auto p = fs::path(Package->Name) / FileName;
  return p.string();
}

std::string SourceFile::FullQualifiedObjectName() const {
  auto p = fs::path(Package->Name) / FileName;
  p.replace_extension(".o");
  return p.string();
}

}  // namespace cc
}  // namespace lang
}  // namespace jk

