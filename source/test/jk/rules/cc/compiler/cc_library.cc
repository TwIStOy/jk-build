// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_library.hh"

#include <memory>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "boost/dll.hpp"
#include "catch.hpp"
#include "jk/common/path.hh"
#include "jk/core/executor/worker_pool.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/interfaces/writer.hh"
#include "jk/core/models/build_package.hh"
#include "jk/core/models/build_package_factory.hh"
#include "jk/core/models/build_rule.hh"
#include "jk/core/models/session.hh"
#include "jk/impls/compilers/makefile/cc_library_compiler.hh"
#include "jk/impls/rules/cc_library.hh"
#include "jk/impls/writers/buffer_writer.hh"
#include "jk/utils/array.hh"
#include "jk/utils/kwargs.hh"
#include "jk/utils/str.hh"
#include "test/jk/core/compile/fake_buffer_writer.hh"
#include "test/jk/core/compile/nop_expander.hh"

namespace jk::impls::cc::test {

static std::unique_ptr<core::models::BuildPackageFactory> SimpleProject() {
  auto factory = std::make_unique<core::models::BuildPackageFactory>();

  {
    auto [package, new_package] = factory->Package("library/base");
    utils::Kwargs kwargs;
    kwargs.value()["name"] = pybind11::str("base");

    auto base = new rules::CCLibrary(package, std::move(kwargs));
    base->Includes.push_back("base_inherit_include_directory");
    base->Defines.push_back("base_inherit_define_flag");
    base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    base->CppFlags.push_back("-Dbase_only_defie_flag");
    base->Headers.push_back("base.h");
  }
  {
    auto [package, new_package] = factory->Package("library/memory");
    utils::Kwargs kwargs;
    kwargs.value()["name"] = pybind11::str("memory");

    auto memory = new rules::CCLibrary(package, std::move(kwargs));

    memory->Dependencies.push_back(
        factory->Package("library/base").first->RulesMap["base"].get());
    memory->Includes.push_back("memory_inherit_include_directory");
    memory->Defines.push_back("memory_inherit_define_flag");
    memory->Sources = {"memory1.cpp", "memory2.cpp", "memory3.cpp",
                       "sub/memory1.cpp"};
    memory->CppFlags.push_back("-Dmemory_only_defie_flag");
  }

  return factory;
}

/*
TEST_CASE("compiler.makefile.cc_library.simple_target",
          "[compiler][makefile][cc_library]") {
  auto compiler = std::make_unique<compilers::makefile::CCLibraryCompiler>();

  core::models::Session session;
  session.Project = std::make_unique<core::filesystem::JKProject>(
      common::AbsolutePath{"~/Projects/test_project"});
  session.PatternExpander = std::make_unique< ::jk::test::NopExpander>();
  session.WriterFactory =
      std::make_unique< ::jk::test::FakeBufferWriterFactory>();
  auto package_factory = SimpleProject();

  auto rule = dynamic_cast<rules::CCLibrary *>(
      package_factory->Package("library/base").first->RulesMap["base"].get());

  SECTION("flags.make") {
    // auto w = writer_factory.Build("flags.make");

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
    core::filesystem::JKProject project{
        common::AbsolutePath{"~/Projects/test_project"},
        core::filesystem::TargetPlatform::k32};

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m32");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("toolchain.make64") {
    core::filesystem::JKProject project{
        common::AbsolutePath{"~/Projects/test_project"},
        core::filesystem::TargetPlatform::k64};

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m64");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("build.make") {
    auto w              = writer_factory.Build("flags.make");
    auto working_folder = project.BuildRoot.Sub(
        utils::Replace(rule->FullQualifiedName(), '/', "@"));

    auto makefile = compiler->GenerateBuild(&project, working_folder, w.get(),
                                            rule, &expander);

    REQUIRE(makefile->Environments["SHELL"].Value == "/bin/bash");
    REQUIRE(makefile->Environments["JK_COMMAND"].Value ==
            boost::dll::program_location().string());
    REQUIRE(makefile->Environments["JK_SOURCE_DIR"].Value ==
            "~/Projects/test_project");
    REQUIRE(makefile->Environments["JK_BINARY_DIR"].Value ==
            "~/Projects/test_project/.build/x86_64");
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
}

TEST_CASE("compiler.makefile.cc_library.target_with_dep",  // {{{
          "[compiler][makefile][cc_library]") {
  core::filesystem::JKProject project{
      common::AbsolutePath{"~/Projects/test_project"},
  };

  ::jk::test::FakeBufferWriterFactory writer_factory;
  auto compiler = std::make_unique<MakefileCCLibraryCompiler>();
  ::jk::test::NopExpander expander;

  auto simple_project = SimpleProject();
  auto rule           = simple_project.Package("library/memory")
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
    core::filesystem::JKProject project{
        common::AbsolutePath{"~/Projects/test_project"},
        core::filesystem::TargetPlatform::k32};

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m32");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("toolchain.make64") {
    core::filesystem::JKProject project{
        common::AbsolutePath{"~/Projects/test_project"},
        core::filesystem::TargetPlatform::k64};

    auto w = writer_factory.Build("flags.make");

    auto makefile = compiler->GenerateToolchain(&project, w.get(), rule);

    REQUIRE(makefile->Environments["CXX"].Value == "g++ -m64");
    REQUIRE(makefile->Environments["AR"].Value == "ar rcs");
    REQUIRE(makefile->Environments["RM"].Value == "$(JK_COMMAND) delete_file");
    REQUIRE(makefile->Environments["LINKER"].Value == "g++");
  }

  SECTION("build.make") {
    auto w              = writer_factory.Build("flags.make");
    auto working_folder = project.BuildRoot.Sub(
        utils::Replace(rule->FullQualifiedName(), '/', "@"));

    auto makefile = compiler->GenerateBuild(&project, working_folder, w.get(),
                                            rule, &expander);

    REQUIRE(makefile->Environments["SHELL"].Value == "/bin/bash");
    REQUIRE(makefile->Environments["JK_COMMAND"].Value ==
            boost::dll::program_location().string());
    REQUIRE(makefile->Environments["JK_SOURCE_DIR"].Value ==
            "~/Projects/test_project");
    REQUIRE(makefile->Environments["JK_BINARY_DIR"].Value ==
            "~/Projects/test_project/.build/x86_64");
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

*/

}  // namespace jk::impls::cc::test

// vim: fdm=marker
