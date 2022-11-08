// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/cc_binary_compiler.hh"

#include <string_view>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_join.h"
#include "jk/core/algorithms/topological_sort.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "range/v3/numeric/iota.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/range/primitives.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/any_view.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/empty.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/for_each.hpp"
#include "range/v3/view/iota.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/single.hpp"
#include "range/v3/view/take.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::impls::compilers::makefile {

static auto logger = utils::Logger("compiler.makefile.cc_library");

auto CCBinaryCompiler::Name() const -> std::string_view {
  return "makefile.cc_binary";
}

static std::vector<std::string> DEBUG_LDFLAGS_BEFORE = {
    "-ftest-coverage",
    "-fprofile-arcs",
};

void CCBinaryCompiler::generate_build_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    rules::CCLibrary *rule) const {
  auto makefile = new_makefile_with_common_commands(session, working_folder);

  makefile.Env(
      "DEBUG_LDFLAGS",
      absl::StrJoin(ranges::views::concat(
                        ranges::views::all(DEBUG_LDFLAGS_BEFORE),
                        ranges::views::all(session->Project->Config().ld_flags),
                        ranges::views::all(
                            session->Project->Config().debug_ld_flags_extra)),
                    " "));

  makefile.Env(
      "RELEASE_LDFLAGS",
      absl::StrJoin(ranges::views::concat(
                        ranges::views::all(session->Project->Config().ld_flags),
                        ranges::views::all(
                            session->Project->Config().release_ld_flags_extra)),
                    " "));

  makefile.Env(
      "PROFILING_LDFLAGS",
      absl::StrJoin(
          ranges::views::concat(
              ranges::views::all(session->Project->Config().ld_flags),
              ranges::views::all(
                  session->Project->Config().profiling_ld_flags_extra)),
          " "));

  core::builder::CustomCommandLines clean_statements;

  // lint headers
  std::vector<std::string> lint_header_targets =
      lint_headers(session, working_folder, rule, &makefile);

  auto binary_progress_num = rule->Steps.Step(".binary");
  for (const auto &build_type : session->BuildTypes) {
    auto all_objects =
        add_source_files_commands(session, working_folder, rule, &makefile,
                                  &lint_header_targets, build_type);
    auto binary_file = working_folder.Sub(build_type, *rule->Base->Name);
    // deps:
    //   all_objects
    //   lint_header_targets
    assert(scc[rule->_scc_id].Rules.size() == 1);

    auto visit_scc = [&](uint32_t id) {
      return scc[id].Rules |
             ranges::views::transform([&](core::models::BuildRule *r) {
               return r->ExportedFiles(session, build_type);
             }) |
             ranges::views::join;
    };
    auto visit_scc_group = [&](uint32_t id) {
      return ranges::views::concat(
          ranges::views::single("-Wl,--start-group"),
          scc[id].Rules |
              ranges::views::transform([&](core::models::BuildRule *r) {
                return ranges::views::concat(
                    r->ExportedFiles(session, build_type),
                    r->ExportedLinkFlags);
              }) |
              ranges::views::join,
          ranges::views::single("-Wl,--end-group"));
    };

    absl::flat_hash_set<uint32_t> visited;
    auto dfs = [&](uint32_t id, auto &&dfs) -> void {
      for (uint32_t n : scc[id].Deps) {
        visited.insert(n);
        dfs(n, dfs);
      }
    };
    auto sorted = core::algorithms::topological_sort(scc);

    auto dependencies_artifact = sorted | ranges::views::filter([&](auto id) {
                                   return visited.contains(id);
                                 }) |
                                 ranges::views::transform([&](auto id) {
                                   return visit_scc(id);
                                 });

    auto deps_and_flags = sorted | ranges::views::filter([&](auto id) {
                            return visited.contains(id);
                          }) |
                          ranges::views::transform([&](auto id) {
                            return visit_scc_group(id);
                          }) |
                          ranges::views::join;

    auto deps = ranges::views::concat(
        all_objects, lint_header_targets,
        dependencies_artifact | ranges::views::join,
        ranges::views::single(working_folder.Sub("build.make").Stringify()));

    auto mkdir_stmt = core::builder::CustomCommandLine::Make(
        {"@$(MKDIR)", binary_file.Path.parent_path().string()});

    auto print_stmt = core::builder::CustomCommandLine::Make(
        {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
         fmt::format("--progress-num={}",
                     absl::StrJoin(rule->Steps.Steps(), ",")),
         fmt::format("--progress-dir={}", session->Project->BuildRoot),
         fmt::format("Linking binary {}", binary_file.Stringify())});

    auto link_stmt = core::builder::CustomCommandLine::FromVec(
        ranges::views::concat(ranges::views::single("@$(LINKER)"), all_objects,
                              deps_and_flags) |
        ranges::to_vector);
    link_stmt.push_back("-g");
    link_stmt.push_back(fmt::format("${{{}_LDFLAGS}}", build_type));
    link_stmt.push_back("-o");
    link_stmt.push_back(binary_file.Stringify());

    makefile.Target(binary_file.Stringify(), deps,
                    ranges::views::concat(ranges::views::single(print_stmt),
                                          ranges::views::single(mkdir_stmt),
                                          ranges::views::single(link_stmt)));

    clean_statements.push_back(core::builder::CustomCommandLine::Make(
        {"@$(RM)", binary_file.Stringify()}));
    for (const auto &obj : all_objects) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"@$(RM)", obj}));
    }
    for (const auto &obj : lint_header_targets) {
      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"@$(RM)", obj}));
    }

    auto build_target = working_folder.Sub(build_type, "build").Stringify();
    makefile.Target(build_target,
                    ranges::views::single(binary_file.Stringify()),
                    ranges::views::empty<core::builder::CustomCommandLine>,
                    "Rule to build all files generated by this target.", true);

    makefile.Target(build_type, ranges::views::single(build_target),
                    ranges::views::empty<core::builder::CustomCommandLine>,
                    "Rule to build all files generated by this target.", true);
  }

  makefile.Target("clean", ranges::views::empty<std::string>,
                  ranges::views::all(clean_statements), "", true);

  end_of_generate_build_file(&makefile, session, working_folder, rule);
}

}  // namespace jk::impls::compilers::makefile
