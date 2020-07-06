// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/common/path.hh"
#include "jk/core/compile/ir.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/writer/writer.hh"

namespace jk {
namespace core {
namespace builder {

struct Builder {
  virtual void WriteIR(filesystem::ProjectFileSystem *project,
                       rules::BuildRule *rule, compile::ir::IR *ir,
                       writer::WriterFactory *writer_factory) = 0;

  virtual ~Builder() = default;
};

}  // namespace builder
}  // namespace core
}  // namespace jk

