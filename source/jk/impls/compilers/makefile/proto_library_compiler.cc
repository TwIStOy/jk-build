// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/proto_library_compiler.hh"

#include <string>
#include <string_view>

#include "absl/strings/str_replace.h"
#include "jk/core/generators/makefile.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/makefile/common.hh"
#include "jk/impls/rules/proto_library.hh"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::makefile {

auto ProtoLibraryCompiler::Name() const -> std::string_view {
  return "makefile.proto_library";
}

struct GeneratedPair {
  common::AbsolutePath Header;
  common::AbsolutePath Source;
};

GeneratedPair add_proto_file_commands(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    core::generators::Makefile *makefile, rules::ProtoLibrary *rule,
    std::string_view filename) {
  auto num = rule->Steps.Step(std::string{filename});

  auto print_stmt =
      PrintStatement(session->Project.get(), "green", false, num,
                     "Compiling proto file {} into .cc/.h", filename);
  auto mkdir = core::builder::CustomCommandLine::Make(
      {"@$(MKDIR)", working_folder.Stringify()});
  auto protoc = core::builder::CustomCommandLine::Make(
      {"${THIRD_PARTY_PROTOBUF_PROTOC}",
       fmt::format("--python_out={}", working_folder.Stringify()),
       fmt::format("--cpp_out={}", working_folder.Stringify()),
       fmt::format("-I{}", session->Project->ProjectRoot.Stringify()),
       std::string(filename)});

  std::map<const absl::string_view, const absl::string_view> replacements{
      {"proto", "pb"}};

  auto gen_cc_file = working_folder.Sub(
      absl::StrReplaceAll(fmt::format("{}.cc", filename), replacements));
  auto gen_h_file = working_folder.Sub(
      absl::StrReplaceAll(fmt::format("{}.h", filename), replacements));

  makefile->Target(
      fmt::format("{} {}", gen_cc_file.Stringify(), gen_h_file.Stringify()),
      ranges::views::single(filename),
      ranges::views::concat(ranges::views::single(print_stmt),
                            ranges::views::single(mkdir),
                            ranges::views::single(protoc)));

  return {gen_h_file, gen_cc_file};
}

void ProtoLibraryCompiler::generate_build_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    rules::CCLibrary *_rule) const {
  (void)scc;
  auto rule = dynamic_cast<rules::ProtoLibrary *>(_rule);

  auto makefile = new_makefile_with_common_commands(session, working_folder);

  makefile.Comment("Sources: ", absl::StrJoin(rule->ExpandedSourceFiles, ", "));

  std::vector<common::AbsolutePath> generated_sources, generated_headers;
  for (auto &filename : rule->ExpandedSourceFiles) {
    auto [header, source] =
        add_proto_file_commands(session, working_folder, &makefile, rule,
                                rule->Package->Path.Sub(filename).Stringify());

    generated_sources.push_back(std::move(source));
    generated_headers.push_back(std::move(header));
  }
  makefile.Comment("Gen-Headers: ",
                   absl::StrJoin(generated_headers, ", ",
                                 [](std::string *output, const auto &x) {
                                   output->append(x.Stringify());
                                 }));
  makefile.Comment("Gen-Sources: ",
                   absl::StrJoin(generated_sources, ", ",
                                 [](std::string *output, const auto &x) {
                                   output->append(x.Stringify());
                                 }));

  std::vector<std::string> lint_header_targets;

  {
    auto library_progress_num = rule->Steps.Step(".library");
    core::builder::CustomCommandLines clean_statements;

    auto source_files =
        generated_sources |
        ranges::views::transform([rule, session](const auto &filename) {
          return std::make_unique<models::cc::SourceFile>(
              common::ProjectRelativePath(fs::relative(
                  filename.Path, session->Project->ProjectRoot.Path)),
              rule);
        }) |
        ranges::to_vector;

    for (const auto &build_type : session->BuildTypes) {
      auto all_objects = add_source_files_commands(
          session, working_folder, rule, &makefile, &lint_header_targets,
          source_files, build_type, true);

      auto library_file_file =
          working_folder.Sub(build_type, rule->LibraryFileName);
      auto library_file = library_file_file.Stringify();
      makefile.Target(library_file, ranges::views::empty<std::string>,
                      ranges::views::empty<core::builder::CustomCommandLine>);

      auto clean_old_library =
          core::builder::CustomCommandLine::Make({"@$(RM)", library_file});

      auto ar_stmt =
          core::builder::CustomCommandLine::Make({"@$(AR)", library_file});
      std::copy(std::begin(all_objects), std::end(all_objects),
                std::back_inserter(ar_stmt));

      auto print_stmt = core::builder::CustomCommandLine::Make(
          {"@$(PRINT)", "--switch=$(COLOR)", "--green", "--bold",
           fmt::format("--progress-num={}",
                       absl::StrJoin(rule->Steps.Steps(), ",")),
           fmt::format("--progress-dir={}",
                       session->Project->BuildRoot.Stringify()),
           fmt::format("Linking CXX static library {}", library_file)});

      makefile.Target(
          library_file,
          ranges::views::concat(
              ranges::views::all(all_objects),
              ranges::views::single(
                  working_folder.Sub("build.make").Stringify()),
              ranges::views::single(
                  working_folder.Sub("toolchain.make").Stringify()),
              ranges::views::single(
                  working_folder.Sub("flags.make").Stringify())),
          ranges::views::concat(
              ranges::views::single(print_stmt),
              ranges::views::single(core::builder::CustomCommandLine::Make(
                  {"@$(MKDIR)",
                   library_file_file.Path.parent_path().string()})),
              ranges::views::single(clean_old_library),
              ranges::views::single(ar_stmt)));

      auto gen_rm = [](const auto &str) {
        return core::builder::CustomCommandLine::Make({"@$(RM)", str});
      };

      clean_statements.push_back(
          core::builder::CustomCommandLine::Make({"@$(RM)", library_file}));
      std::transform(std::begin(all_objects), std::end(all_objects),
                     std::back_inserter(clean_statements), gen_rm);

      auto build_target = working_folder.Sub(build_type, "build").Stringify();
      makefile.Target(build_target, ranges::views::single(library_file),
                      ranges::views::empty<core::builder::CustomCommandLine>,
                      "Rule to build all files generated by this target.",
                      true);

      makefile.Target(build_type, ranges::views::single(build_target),
                      ranges::views::empty<core::builder::CustomCommandLine>,
                      "Rule to build all files generated by this target.",
                      true);
    }

    makefile.Target("clean", ranges::views::empty<std::string>,
                    ranges::views::all(clean_statements), "", true);
  }

  end_of_generate_build_file(&makefile, session, working_folder, rule);
}

}  // namespace jk::impls::compilers::makefile
