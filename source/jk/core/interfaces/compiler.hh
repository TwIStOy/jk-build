// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <string_view>

#include "jk/core/models/build_rule.hh"
#include "jk/core/models/session.hh"

namespace jk::core::interfaces {

struct Compiler {
  virtual ~Compiler() = default;

  //! Name of a compiler. Should in format: `{{OUTPUT_FORMAT}}.{{RULE_TYPE}}`,
  //! like 'Makefile.cc_library'
  virtual std::string_view Name() const = 0;

  //! Compile a rule.
  virtual void Compile(models::Session *session,
                       models::BuildRule *rule) const = 0;
};

}  // namespace jk::core::interfaces
