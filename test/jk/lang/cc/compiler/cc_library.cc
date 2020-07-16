// Copyright (c) 2020 Hawtian Wang
//

#include "jk/lang/cc/compiler/cc_library.hh"

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

static bool SameArray(const std::string &str,
                      const std::vector<std::string> &vec) {
  std::cout << fmt::format(R"(SameArray checking "{}" and [{}])", str,
                           utils::JoinString(", ", vec.begin(), vec.end()))
            << std::endl;

  std::set<std::string> parts;
  std::set<std::string> parts2{vec.begin(), vec.end()};

  std::stringstream iss(str);
  std::string line;
  while (std::getline(iss, line, ' ')) {
    parts.insert(line);
  }

  if (vec.size() != parts.size()) {
    return false;
  }

  auto it1 = parts.begin();
  auto it2 = parts2.begin();
  for (; it1 != parts.end(); ++it1, ++it2) {
    if (*it1 != *it2) {
      return false;
    }
  }

  return true;
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

    REQUIRE(SameArray(makefile->Environments["C_FLAGS"].Value, {}));
    REQUIRE(SameArray(makefile->Environments["CXX_FLAGS"].Value, {}));
    REQUIRE(SameArray(makefile->Environments["CPP_FLAGS"].Value,
                      {"-Dbase_only_defie_flag"}));
    REQUIRE(SameArray(makefile->Environments["CXX_DEFINE"].Value,
                      {"-Dbase_inherit_define_flag"}));
    REQUIRE(SameArray(makefile->Environments["CXX_INCLUDE"].Value,
                      {"-Ibase_inherit_include_directory"}));
  }
}

}  // namespace jk::lang::cc::test

// vim: fdm=marker

