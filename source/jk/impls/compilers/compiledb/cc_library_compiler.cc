// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/compilers/compiledb/cc_library_compiler.hh"

#include <vector>

#include "jk/core/generators/compiledb.hh"
#include "jk/core/models/build_package.hh"
#include "jk/impls/models/cc/source_file.hh"
#include "jk/impls/rules/cc_library.hh"
#include "nlohmann/json.hpp"
#include "range/v3/view/all.hpp"
#include "range/v3/view/concat.hpp"
#include "range/v3/view/single.hpp"

namespace jk::impls::compilers::compiledb {

auto CCLibraryCompiler::Name() const -> std::string_view {
  return "compiledb.cc_library";
}

auto CCLibraryCompiler::Compile(
    core::models::Session *session,
    const std::vector<core::algorithms::StronglyConnectedComponent> &scc,
    core::models::BuildRule *_rule) const -> void {
  auto rule           = dynamic_cast<rules::CCLibrary *>(_rule);
  auto working_folder = rule->WorkingFolder;
  auto directory      = session->Project->ProjectRoot.Stringify();

  auto CXX = ranges::views::concat(
      session->Project->Config().cxx,
      ranges::views::single(session->Project->Platform ==
                                    core::filesystem::TargetPlatform::k64
                                ? "-m64"
                                : "-m32"));

  auto CC = ranges::views::concat(
      session->Project->Config().cxx,
      ranges::views::single(session->Project->Platform ==
                                    core::filesystem::TargetPlatform::k64
                                ? "-m64"
                                : "-m32"));

  auto CPPFLAGS = ranges::views::all(rule->ExpandedCFileFlags);
  auto CXXFLAGS = ranges::views::concat(rule->CxxFlags, session->ExtraFlags);
  auto INHERENT_FLAGS = ranges::views::all(rule->ResolvedInherentFlags);
  auto CFLAGS         = ranges::views::all(rule->ExpandedCFileFlags);
  auto CPP_DEFINES    = ranges::views::all(rule->ResolvedDefines);
  auto CPP_INCLUDES   = ranges::views::all(rule->ResolvedIncludes);
  auto compile_flags =
      ranges::views::concat(session->Project->Config().compile_flags,
                            ranges::views::single("-DGIT_DESC"));
  auto cppincludes = ranges::views::concat(
      ranges::views::single("-isystem"),
      ranges::views::single(fmt::format(
          ".build/.lib/m{}/include",
          session->Project->Platform == core::filesystem::TargetPlatform::k64
              ? "64"
              : "32")),
      ranges::views::single("-I.build/include"));
  static auto cxxincludes = ranges::views::single("-I.build/pb/c++");

  auto DEBUG_CXX_FLAGS =
      ranges::views::concat(compile_flags, session->Project->Config().cxxflags,
                            session->Project->Config().release_cxxflags_extra,
                            cppincludes, cxxincludes);
  auto DEBUG_C_FLAGS = ranges::views::concat(
      compile_flags, session->Project->Config().cflags,
      session->Project->Config().release_cflags_extra, cppincludes);

  std::vector<nlohmann::json> values;
  for (const auto &s : rule->ExpandedSourceFiles) {
    auto source_file = models::cc::SourceFile(s, rule);

    auto filename =
        source_file.ResolveFullQualifiedPath(session->Project->ProjectRoot)
            .Stringify();

    nlohmann::json value;
    value["directory"] = directory;
    value["file"]      = filename;

    if (source_file.IsCSourceFile) {
      value["arguments"] =
          ranges::views::concat(CXX, CPP_DEFINES, CPP_INCLUDES, CPPFLAGS,
                                CXXFLAGS, DEBUG_CXX_FLAGS, INHERENT_FLAGS,
                                ranges::views::single("-c"),
                                ranges::views::single(filename)) |
          ranges::to_vector;
    } else {
      value["arguments"] =
          ranges::views::concat(CC, CPP_DEFINES, CPP_INCLUDES, CPPFLAGS, CFLAGS,
                                DEBUG_C_FLAGS, INHERENT_FLAGS,
                                ranges::views::single("-c"),
                                ranges::views::single(filename)) |
          ranges::to_vector;
    }
    values.push_back(std::move(value));
  }

  session->CompilationDatabase->AddValues(std::move(values));
}

}  // namespace jk::impls::compilers::compiledb
