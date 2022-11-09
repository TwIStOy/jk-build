// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/shell_script_compiler.hh"

#include "jk/core/generators/makefile.hh"
#include "jk/core/models/build_package.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "jk/impls/rules/shell_script.hh"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

static const char *ExternalInstalledPrefix = ".build/.lib/m${PLATFORM}";

auto ShellScriptCompiler::Name() const -> std::string_view {
  return "makefile.shell_script";
}

auto ShellScriptCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    core::models::BuildRule *_rule) const -> void {
  auto rule           = dynamic_cast<rules::ShellScript *>(_rule);
  auto working_folder = session->Project->BuildRoot;

  auto makefile = new_makefile_with_common_commands(
      session, rule->WorkingFolder, "build.make", true);

  makefile.Env("JK_COMMAND", "jk");

  makefile.Env("MKDIR", "mkdir -p");

  makefile.Env("RM", "$(JK_COMMAND) delete_file",
               "The command to remove a file.");

  auto script_target = rule->WorkingFolder.Sub("CHECK_POINT").Stringify();
  {
    // script target
    auto print_stmt = PrintStatement(
        session->Project.get(), "green", true, rule->Steps.Step("execute"),
        "Installing External Project {}", rule->Base->FullQualifiedName);

    auto mkdir_stmt = core::builder::CustomCommandLine::Make(
        {"@$(MKDIR)", session->Project->ProjectRoot.Sub(ExternalInstalledPrefix)
                          .Stringify()});

    auto run_stmt = core::builder::CustomCommandLine::Make(
        {fmt::format("@{}/{}", session->Project->Resolve(rule->Package->Path),
                     rule->Script),
         ToString(session->Project->Platform)});
    auto touch_stmt =
        core::builder::CustomCommandLine::Make({"@touch", script_target});

    makefile.Target(
        script_target,
        ranges::views::single(
            session->Project->Resolve(rule->Package->Path, rule->Script)
                .Stringify()),
        core::builder::CustomCommandLines::Multiple(print_stmt, mkdir_stmt,
                                                    run_stmt, touch_stmt));
  }

  {
    // clean target
    core::builder::CustomCommandLines lines;

    for (auto &it : rule->Artifacts) {
      lines.push_back(core::builder::CustomCommandLine::Make({"@$(RM)", it}));
    }
    lines.push_back(
        core::builder::CustomCommandLine::Make({"@$(RM)", script_target}));

    makefile.Target("clean", ranges::views::empty<std::string>, lines);
  }

  for (auto &it : rule->Artifacts) {
    makefile.Target(it, ranges::views::single(script_target),
                    ranges::views::empty<core::builder::CustomCommandLine>);
  }

  makefile.Target("build", rule->Artifacts,
                  ranges::views::empty<core::builder::CustomCommandLine>, "",
                  true);
}

}  // namespace jk::impls::compilers::makefile
