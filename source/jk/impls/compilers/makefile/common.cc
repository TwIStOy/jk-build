// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/common.hh"

#include "jk/core/generators/makefile.hh"
#include "jk/core/models/session.hh"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

core::generators::Makefile new_makefile_with_common_commands(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    std::string_view filename, bool no_include) {
  core::generators::Makefile makefile(working_folder.Sub(filename),
                                      {session->WriterFactory.get()});

  makefile.Env("SHELL", "/bin/bash",
               "The shell in which to execute make rules.");

  makefile.Env("JK_COMMAND", session->JKPath, "The command Jk executable.");

  makefile.Env("JK_SOURCE_DIR", session->Project->ProjectRoot.Stringify(),
               "The top-level source directory on which Jk was run.");

  makefile.Env("JK_BINARY_DIR", session->Project->BuildRoot.Stringify(),
               "The top-level build directory on which Jk was run.");

  makefile.Env("JK_BUNDLE_LIBRARY_PREFIX",
               session->Project->ExternalInstalledPrefix.Stringify(),
               "The bundled libraries prefix on which Jk was run.");

  makefile.Env("EQUALS", "=", "Escaping for special characters.");

  makefile.Env("PRINT", session->JKPath + " echo_color");

  makefile.Env("JK_VERBOSE_FLAG", "V$(VERBOSE)");

  if (session->Project->Platform == core::filesystem::TargetPlatform::k32) {
    makefile.Env("PLATFORM", "32");
  } else {
    makefile.Env("PLATFORM", "64");
  }

  makefile.Target(core::generators::Makefile::DEFAULT_TARGET,
                  ranges::views::empty<std::string>,
                  ranges::views::empty<core::builder::CustomCommandLine>);

  if (!no_include) {
    makefile.Include(working_folder.Sub("flags.make").Path.string(),
                     "Include the compile flags for this rule's objectes.",
                     true);
    makefile.Include(working_folder.Sub("toolchain.make").Path.string(),
                     "Include used toolchains for this rule's objects.", true);
  }

  makefile.Target("all", ranges::views::single("DEBUG"),
                  ranges::views::empty<core::builder::CustomCommandLine>, "",
                  true);
  makefile.Target(
      "all", ranges::views::single(core::generators::Makefile::DEFAULT_TARGET),
      ranges::views::empty<core::builder::CustomCommandLine>);

  makefile.Target("jk_force", ranges::views::empty<std::string>,
                  ranges::views::empty<core::builder::CustomCommandLine>,
                  "This target is always outdated.", true);

  return makefile;
}

}  // namespace jk::impls::compilers::makefile
