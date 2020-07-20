// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_binary.hh"

#include "catch.hpp"
#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/buffer_writer.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_binary.hh"
#include "jk/lang/cc/rules/cc_library.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"
#include "test/jk/core/compile/fake_buffer_writer.hh"
#include "test/jk/core/compile/nop_expander.hh"
#include "test/jk/lang/cc/compiler/utility.hh"

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
    base->LdFlags.push_back("-lbase_ldflags");
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
    memory->LdFlags.push_back("-lmemory_ldflags");
    memory->CppFlags.push_back("-Dmemory_only_defie_flag");
  }
  {
    auto package = factory.Package("application/app");
    auto app = new core::rules::CCBinary(package, "app");
    app->Dependencies.push_back(
        factory.Package("library/base")->Rules["base"].get());
    app->Dependencies.push_back(
        factory.Package("library/memory")->Rules["memory"].get());

    app->Includes.push_back("app_inherit_include_directory");
    app->Defines.push_back("app_inherit_define_flag");
    app->Sources = {"main.cpp"};
    app->CppFlags.push_back("-Dapp_only_defie_flag");

    app->LdFlags.push_back("-lapp_ld_flags_1");
    app->LdFlags.push_back("-lapp_ld_flags_2");
    app->LdFlags.push_back("-lapp_ld_flags_3");
  }

  return factory;
}

TEST_CASE("compiler.makefile.cc_binary.target_with_dep",
          "[compiler][makefile][cc_binary]") {
  core::filesystem::ProjectFileSystem project{
      common::AbsolutePath{"~/Projects/test_project"},
      common::AbsolutePath{"~/Projects/test_project/.build"},
  };

  ::jk::test::FakeBufferWriterFactory writer_factory;
  auto compiler = std::make_unique<MakefileCCBinaryCompiler>();
  ::jk::test::NopExpander expander;

  auto simple_project = SimpleProject();
  auto rule = simple_project.Package("application/app")
                  ->Rules["app"]
                  ->Downcast<core::rules::CCBinary>();

  SECTION("flags.make") {
    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateFlags(w.get(), rule);

    REQUIRE(utils::SameArray(makefile->Environments["C_FLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(utils::SameArray(makefile->Environments["CXX_FLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(
        utils::SameArray(makefile->Environments["CPP_FLAGS"].Value,
                         std::vector<std::string>{"-Dapp_only_defie_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CXX_DEFINE"].Value,
        std::vector<std::string>{"-Dbase_inherit_define_flag",
                                 "-Dmemory_inherit_define_flag",
                                 "-Dapp_inherit_define_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CXX_INCLUDE"].Value,
        std::vector<std::string>{"-Ibase_inherit_include_directory",
                                 "-Imemory_inherit_include_directory",
                                 "-Iapp_inherit_include_directory"}));
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
            "~/Projects/test_project");
    REQUIRE(makefile->Environments["JK_BINARY_DIR"].Value ==
            "~/Projects/test_project/.build");
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

    auto main_object = working_folder.Sub("application/app/main.o").Stringify();
    auto main_deps = MergeDependencies(makefile->Targets, main_object);

    UNSCOPED_INFO(fmt::format(
        "dep: [{}]",
        utils::JoinString(", ", std::begin(main_deps), std::end(main_deps))));

    REQUIRE(utils::SameArray(
        main_deps, std::vector<std::string>{
                       working_folder.Sub("flags.make").Stringify(),
                       working_folder.Sub("toolchain.make").Stringify(),
                       "~/Projects/test_project/application/app/main.cpp",
                       "~/Projects/test_project/library/base/base.h",
                   }));

    auto exec_target = working_folder.Sub(rule->ExportedFileName).Stringify();
    auto exec_deps = MergeDependencies(makefile->Targets, exec_target);

    UNSCOPED_INFO(fmt::format(
        "exec dep: [{}]",
        utils::JoinString(", ", std::begin(exec_deps), std::end(exec_deps))));
    REQUIRE(utils::SameArray(
        exec_deps,
        std::vector<std::string>{
            working_folder.Sub("application/app/main.o").Stringify(),
            project.BuildRoot
                .Sub(utils::Replace(simple_project.Package("library/base")
                                        ->Rules["base"]
                                        .get()
                                        ->FullQualifiedName(),
                                    '/', "@"))
                .Sub(simple_project.Package("library/base")
                         ->Rules["base"]
                         .get()
                         ->Downcast<core::rules::CCLibrary>()
                         ->ExportedFileName)
                .Stringify(),
            project.BuildRoot
                .Sub(utils::Replace(simple_project.Package("library/memory")
                                        ->Rules["memory"]
                                        .get()
                                        ->FullQualifiedName(),
                                    '/', "@"))
                .Sub(simple_project.Package("library/memory")
                         ->Rules["memory"]
                         .get()
                         ->Downcast<core::rules::CCLibrary>()
                         ->ExportedFileName)
                .Stringify(),
        }));
  }

  SECTION("view all") {
    compiler->Compile(&project, &writer_factory, rule, &expander);
    writer_factory.DebugPrint(std::cout);
  }
}

}  // namespace jk::lang::cc::test

// vim: fdm=marker

