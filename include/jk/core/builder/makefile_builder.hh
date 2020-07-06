// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <ostream>
#include <string>
#include <unordered_map>

#include "jk/core/builder/builder.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/writer/writer.hh"

namespace jk {
namespace core {
namespace builder {

struct MakefileBuilder final : Builder {
  void WriteIR(filesystem::ProjectFileSystem *project, rules::BuildRule *rule,
               compile::ir::IR *ir,
               writer::WriterFactory *writer_factory) final;
};

}  // namespace builder
}  // namespace core
}  // namespace jk

