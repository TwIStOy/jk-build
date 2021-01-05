// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>

#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"

namespace jk::core::compile {

struct PreProcesser {
  //! Name of a PreProcesser. It is same as `{{RULE_TYPE}}`.
  virtual std::string Name() const = 0;

  virtual void PreProcess(filesystem::JKProject *project,
                          rules::BuildRule *rule,
                          filesystem::FileNamePatternExpander *expander =
                              &filesystem::kDefaultPatternExpander) const = 0;
};

}  // namespace jk::core::compile

// vim: fdm=marker
