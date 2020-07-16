// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_library.hh"

#include <boost/optional.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "catch.hpp"
#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/buffer_writer.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"
#include "test/jk/core/compile/fake_buffer_writer.hh"
#include "test/jk/core/compile/nop_expander.hh"

namespace jk::lang::cc::test {

static core::rules::BuildPackageFactory SimpleProject() {
  core::rules::BuildPackageFactory factory;

  {
    auto package = factory.Package("library/base");
    auto base = new core::rules::CCLibrary(package, "base");
    base->Includes.push_back("base_inherit_include_directory");
    base->Defines.push_back("base_inherit_define_flag");
    base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    base->CppFlags.push_back("-Dbase_only_defie_flag");
    base->Headers.push_back("base.h");
  }
  {
    auto package = factory.Package("library/memory");
    auto memory = new core::rules::CCLibrary(package, "memory");
    memory->Dependencies.push_back(
        factory.Package("library/base")->Rules["base"].get());
    memory->Includes.push_back("memory_inherit_include_directory");
    memory->Defines.push_back("memory_inherit_define_flag");
    memory->Sources = {"memory1.cpp", "memory2.cpp", "memory3.cpp",
                       "sub/memory1.cpp"};
    memory->CppFlags.push_back("-Dmemory_only_defie_flag");
  }

  return factory;
}

static boost::optional<core::output::UnixMakefile::IncludeItem> FindInclude(
    std::list<core::output::UnixMakefile::IncludeItem> includes,
    const fs::path &path) {
  for (const auto &it : includes) {
    if (it.Path == path) {
      return it;
    }
  }
  return {};
}

static std::list<std::string> MergeDependencies(
    const std::list<core::output::UnixMakefile::TargetItem> &targets,
    const std::string &name) {
  std::list<std::string> res;
  for (const auto &target : targets) {
    if (target.TargetName == name) {
      res.insert(res.end(), target.Dependencies.begin(),
                 target.Dependencies.end());
    }
  }
  return res;
}

TEST_CASE("compiler.makefile.cc_library.simple_target",
          "[compiler][makefile][cc_library]") {
  core::filesystem::ProjectFileSystem project{
      common::AbsolutePath{"~/Projects/agora"},
      common::AbsolutePath{"~/Projects/agora/.build"},
  };

  ::jk::test::FakeBufferWriterFactory writer_factory;
  auto compiler = std::make_unique<MakefileCCLibraryCompiler>();
  ::jk::test::NopExpander expander;

  auto simple_project = SimpleProject();
  auto rule = simple_project.Package("library/base")
                  ->Rules["base"]
                  ->Downcast<core::rules::CCLibrary>();

  SECTION("flags.make") {
    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateFlags(w.get(), rule);

    REQUIRE(utils::SameArray(makefile->Environments["C_FLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(utils::SameArray(makefile->Environments["CXX_FLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(
        utils::SameArray(makefile->Environments["CPP_FLAGS"].Value,
                         std::vector<std::string>{"-Dbase_only_defie_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CXX_DEFINE"].Value,
        std::vector<std::string>{"-Dbase_inherit_define_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CXX_INCLUDE"].Value,
        std::vector<std::string>{"-Ibase_inherit_include_directory"}));
  }

  SECTION("toolchain.make") {
    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(w.get());

    REQUIRE(makefile->Environments["CXX"].Value == "g++");
    REQUIRE(makefile->Environments["AR"].Value == "ar qc");
    REQUIRE(makefile->Environments["RM"].Value == "rm");
  }

  SECTION("build.make") {
    auto w = writer_factory.Build("flags.make");
    auto working_folder = project.BuildRoot.Sub(
        utils::Replace(rule->FullQualifiedName(), '/', "@"));

    auto makefile = compiler->GenerateBuild(&project, working_folder, w.get(),
                                            rule, &expander);

    REQUIRE(makefile->Environments["SHELL"].Value == "/bin/bash");
    REQUIRE(makefile->Environments["JK_COMMAND"].Value == "jk");
    REQUIRE(makefile->Environments["JK_SOURCE_DIR"].Value ==
            "~/Projects/agora");
    REQUIRE(makefile->Environments["JK_BINARY_DIR"].Value ==
            "~/Projects/agora/.build");
    REQUIRE(makefile->Environments["EQUALS"].Value == "=");
    REQUIRE(makefile->Environments["PRINT"].Value == "jk tools echo_color");

    auto include_flags_make =
        FindInclude(makefile->Includes, working_folder.Sub("flags.make").Path);
    REQUIRE(include_flags_make.is_initialized());
    CHECK(include_flags_make.value().Fatal == true);

    auto include_toolchain_make = FindInclude(
        makefile->Includes, working_folder.Sub("toolchain.make").Path);
    REQUIRE(include_toolchain_make.is_initialized());
    CHECK(include_toolchain_make.value().Fatal == true);

    // base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    auto base1_object = working_folder.Sub("library/base/base1.o").Stringify();
    auto base1_dep = MergeDependencies(makefile->Targets, base1_object);
    // TODO(hawtian): fix test case
    REQUIRE(utils::SameArray(base1_dep, std::vector<std::string>{}));
  }
}

}  // namespace jk::lang::cc::test

// vim: fdm=marker

