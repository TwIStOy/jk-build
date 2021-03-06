// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_library.hh"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "catch.hpp"
#include "jk/common/flags.hh"
#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/buffer_writer.hh"
#include "jk/core/writer/writer.hh"
#include "jk/rules/cc/compiler/cc_library.hh"
#include "jk/rules/cc/rules/cc_library_helper.hh"
#include "jk/rules/cc/source_file.hh"
#include "jk/utils/array.hh"
#include "jk/utils/str.hh"
#include "test/jk/core/compile/fake_buffer_writer.hh"
#include "test/jk/core/compile/nop_expander.hh"
#include "test/jk/rules/cc/compiler/utility.hh"

namespace jk::rules::cc::test {

static core::rules::BuildPackageFactory SimpleProject() {
  SourceFile::ClearCache();
  core::rules::BuildPackageFactory factory;

  {
    auto package = factory.Package("library/base");
    auto base = new CCLibrary(package, "base");
    base->Includes.push_back("base_inherit_include_directory");
    base->Defines.push_back("base_inherit_define_flag");
    base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    base->CppFlags.push_back("-Dbase_only_defie_flag");
    base->Headers.push_back("base.h");
  }
  {
    auto package = factory.Package("library/memory");
    auto memory = new CCLibrary(package, "memory");
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

TEST_CASE("compiler.makefile.cc_library.simple_target",  // {{{
          "[compiler][makefile][cc_library]") {
  core::filesystem::JKProject project{
      common::AbsolutePath{"~/Projects/test_project"},
      common::AbsolutePath{"~/Projects/test_project/.build"},
  };

  ::jk::test::FakeBufferWriterFactory writer_factory;
  auto compiler =
      std::make_unique<::jk::rules::cc::MakefileCCLibraryCompiler>();
  ::jk::test::NopExpander expander;

  auto simple_project = SimpleProject();
  auto rule = simple_project.Package("library/base")
                  ->Rules["base"]
                  ->Downcast<CCLibrary>();

  SECTION("flags.make") {
    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateFlags(&project, w.get(), rule);

    REQUIRE(utils::SameArray(makefile->Environments["CFLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(utils::SameArray(makefile->Environments["CXXFLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(
        utils::SameArray(makefile->Environments["CPPFLAGS"].Value,
                         std::vector<std::string>{"-Dbase_only_defie_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CPP_DEFINES"].Value,
        std::vector<std::string>{"-Dbase_inherit_define_flag"}));

    REQUIRE(utils::SameArray(
        makefile->Environments["CPP_INCLUDES"].Value,
        std::vector<std::string>{"-Ibase_inherit_include_directory"}));
  }

  SECTION("toolchain.make32") {
    common::FLAGS_platform = common::Platform::k32;

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m32");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("toolchain.make64") {
    common::FLAGS_platform = common::Platform::k64;

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m64");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
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
    REQUIRE(makefile->Environments["PRINT"].Value == "jk echo_color");

    auto include_flags_make =
        FindInclude(makefile->Includes, working_folder.Sub("flags.make").Path);
    REQUIRE(include_flags_make.is_initialized());
    CHECK(include_flags_make.value().Fatal == true);

    auto include_toolchain_make = FindInclude(
        makefile->Includes, working_folder.Sub("toolchain.make").Path);
    REQUIRE(include_toolchain_make.is_initialized());
    CHECK(include_toolchain_make.value().Fatal == true);

    // base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    auto base1_object =
        working_folder.Sub("DEBUG/library/base/base1.cpp.o").Stringify();
    auto base1_dep = MergeDependencies(makefile->Targets, base1_object);

    REQUIRE(utils::SameArray(
        base1_dep,
        std::vector<std::string>{
            working_folder.Sub("flags.make").Stringify(),
            working_folder.Sub("toolchain.make").Stringify(),
            "~/Projects/test_project/library/base/base1.cpp",
            working_folder.Sub("library/base/base1.cpp.lint").Stringify(),
        }));

    auto library_target =
        working_folder.Sub("DEBUG").Sub(rule->ExportedFileName).Stringify();
    auto library_dep = MergeDependencies(makefile->Targets, library_target);
    REQUIRE(
        utils::SameArray(library_dep, std::vector<std::string>{
                                          working_folder.Sub("DEBUG")
                                              .Sub("library/base/base1.cpp.o")
                                              .Stringify(),
                                          working_folder.Sub("DEBUG")
                                              .Sub("library/base/base2.cpp.o")
                                              .Stringify(),
                                          working_folder.Sub("DEBUG")
                                              .Sub("library/base/base3.cpp.o")
                                              .Stringify(),
                                      }));
  }

  SECTION("view all") {
    compiler->Compile(&project, &writer_factory, rule, &expander);
    writer_factory.DebugPrint(std::cout);
  }
}  // }}}

TEST_CASE("compiler.makefile.cc_library.target_with_dep",  // {{{
          "[compiler][makefile][cc_library]") {
  core::filesystem::JKProject project{
      common::AbsolutePath{"~/Projects/test_project"},
      common::AbsolutePath{"~/Projects/test_project/.build"},
  };

  ::jk::test::FakeBufferWriterFactory writer_factory;
  auto compiler = std::make_unique<MakefileCCLibraryCompiler>();
  ::jk::test::NopExpander expander;

  auto simple_project = SimpleProject();
  auto rule = simple_project.Package("library/memory")
                  ->Rules["memory"]
                  ->Downcast<CCLibrary>();

  SECTION("flags.make") {
    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateFlags(&project, w.get(), rule);

    REQUIRE(utils::SameArray(makefile->Environments["CFLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(utils::SameArray(makefile->Environments["CXXFLAGS"].Value,
                             std::vector<std::string>{}));
    REQUIRE(
        utils::SameArray(makefile->Environments["CPPFLAGS"].Value,
                         std::vector<std::string>{"-Dmemory_only_defie_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CPP_DEFINES"].Value,
        std::vector<std::string>{"-Dbase_inherit_define_flag",
                                 "-Dmemory_inherit_define_flag"}));
    REQUIRE(utils::SameArray(
        makefile->Environments["CPP_INCLUDES"].Value,
        std::vector<std::string>{"-Ibase_inherit_include_directory",
                                 "-Imemory_inherit_include_directory"}));
  }

  SECTION("toolchain.make32") {
    common::FLAGS_platform = common::Platform::k32;

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m32");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("toolchain.make64") {
    common::FLAGS_platform = common::Platform::k64;

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m64");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
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
    REQUIRE(makefile->Environments["PRINT"].Value == "jk echo_color");

    auto include_flags_make =
        FindInclude(makefile->Includes, working_folder.Sub("flags.make").Path);
    REQUIRE(include_flags_make.is_initialized());
    CHECK(include_flags_make.value().Fatal == true);

    auto include_toolchain_make = FindInclude(
        makefile->Includes, working_folder.Sub("toolchain.make").Path);
    REQUIRE(include_toolchain_make.is_initialized());
    CHECK(include_toolchain_make.value().Fatal == true);

    auto base1_object =
        working_folder.Sub("DEBUG/library/memory/memory1.cpp.o").Stringify();
    auto base1_dep = MergeDependencies(makefile->Targets, base1_object);
    UNSCOPED_INFO(fmt::format(
        "dep: [{}]",
        utils::JoinString(", ", base1_dep.begin(), base1_dep.end())));

    REQUIRE(utils::SameArray(
        base1_dep,
        std::vector<std::string>{
            working_folder.Sub("flags.make").Stringify(),
            working_folder.Sub("toolchain.make").Stringify(),
            "~/Projects/test_project/library/memory/memory1.cpp",
            "~/Projects/test_project/library/base/base.h",
            working_folder.Sub("library/memory/memory1.cpp.lint").Stringify(),
        }));

    auto library_target =
        working_folder.Sub("DEBUG").Sub(rule->ExportedFileName).Stringify();
    auto library_dep = MergeDependencies(makefile->Targets, library_target);
    UNSCOPED_INFO(
        fmt::format("library_dep: [{}]", utils::JoinString(", ", library_dep)));
    REQUIRE(utils::SameArray(
        library_dep,
        std::vector<std::string>{
            working_folder.Sub("DEBUG/library/memory/memory1.cpp.o")
                .Stringify(),
            working_folder.Sub("DEBUG/library/memory/memory2.cpp.o")
                .Stringify(),
            working_folder.Sub("DEBUG/library/memory/memory3.cpp.o")
                .Stringify(),
            working_folder.Sub("DEBUG/library/memory/sub/memory1.cpp.o")
                .Stringify(),
        }));
  }

  SECTION("view all") {
    compiler->Compile(&project, &writer_factory, rule, &expander);
    writer_factory.DebugPrint(std::cout);
  }
}  // }}}

}  // namespace jk::rules::cc::test

// vim: fdm=marker
