// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/makefile/proto_library_compiler.hh"

#include <string>
#include <string_view>

#include "jk/core/generators/makefile.hh"
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
  std::string Header;
  std::string Source;
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

  auto gen_cc_file = fmt::format("{}.cc", filename);
  auto gen_h_file  = fmt::format("{}.h", filename);

  makefile->Target(fmt::format("{} {}", gen_cc_file, gen_h_file),
                   ranges::views::single(filename),
                   ranges::views::concat(ranges::views::single(print_stmt),
                                         ranges::views::single(mkdir),
                                         ranges::views::single(protoc)));

  return {gen_h_file, gen_cc_file};
}

void ProtoLibraryCompiler::generate_build_file(
    core::models::Session *session, const common::AbsolutePath &working_folder,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    rules::CCLibrary *_rule) {
  (void)scc;
  auto rule = dynamic_cast<rules::ProtoLibrary *>(_rule);

  auto makefile = new_makefile_with_common_commands(session, working_folder);

  makefile.Comment("Sources: ", absl::StrJoin(rule->ExpandedSourceFiles, ", "));

  std::vector<std::string> generated_sources, generated_headers;
  for (auto &filename : rule->ExpandedSourceFiles) {
    auto [header, source] = add_proto_file_commands(session, working_folder,
                                                    &makefile, rule, filename);
    generated_sources.push_back(std::move(source));
    generated_headers.push_back(std::move(header));
  }
  makefile.Comment("Gen-Headers: ", absl::StrJoin(generated_headers, ", "));
  makefile.Comment("Gen-Sources: ", absl::StrJoin(generated_sources, ", "));

  auto source_files =
      generated_sources |
      ranges::views::transform([rule](const auto &filename) {
        return std::make_unique<models::cc::SourceFile>(filename, rule);
      }) |
      ranges::to_vector;

  for (const auto &build_type : session->BuildTypes) {
    auto all_objects = add_source_files_commands(
        session, working_folder, rule, &makefile, &lint_header_targets,
        source_files, build_type);
  }
}

}  // namespace jk::impls::compilers::makefile
