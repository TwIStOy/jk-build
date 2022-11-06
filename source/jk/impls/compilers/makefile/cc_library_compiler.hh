// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <string_view>

#include "jk/core/generators/makefile.hh"
#include "jk/core/interfaces/compiler.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/rules/cc_library.hh"

namespace jk::impls::compilers::makefile {

struct CCLibraryCompiler : core::interfaces::Compiler {
  std::string_view Name() const override;

  void Compile(core::models::Session *session,
               core::models::BuildRule *rule) const override;

 protected:
  virtual void generate_flag_file(core::models::Session *session,
                                  const common::AbsolutePath &working_folder,
                                  rules::CCLibrary *rule) const;

  virtual void generate_toolchain_file(
      core::models::Session *session,
      const common::AbsolutePath &working_folder, rules::CCLibrary *rule) const;

  virtual void generate_build_file(core::models::Session *session,
                                   const common::AbsolutePath &working_folder,
                                   rules::CCLibrary *rule) const;

  virtual void end_of_generate_build_file(
      core::generators::Makefile *makefile, core::models::Session *session,
      const common::AbsolutePath &working_folder,
      rules::CCLibrary *rule) const {
    (void)makefile;
    (void)session;
    (void)working_folder;
    (void)rule;
  }

 private:
  void DoCompile(core::models::Session *session, rules::CCLibrary *rule) const;
};

}  // namespace jk::impls::compilers::makefile
