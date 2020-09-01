// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/compiler/proto_library.hh"

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

  // TODO(hawtian):
}

}  // namespace jk::rules::cc

// vim: fdm=marker

