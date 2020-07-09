// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/compile.hh"

#include <fmt/format.h>

#include <catch.hpp>
#include <iostream>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

#include "jk/common/path.hh"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule.hh"
#include "jk/core/rules/package.hh"
#include "jk/core/writer/buffer_writer.hh"
#include "jk/core/writer/writer.hh"
#include "jk/lang/cc/rules/cc_library.hh"

namespace jk::core::compile::test {

struct FakeBufferWriterFactory;
struct FakeBufferWriter final : public writer::BufferWriter {
  FakeBufferWriter(FakeBufferWriterFactory *factory, std::string key)
      : Key(key), MyFactory(factory) {
  }

  virtual ~FakeBufferWriter();

  std::string Key;
  FakeBufferWriterFactory *MyFactory;
};

struct FakeBufferWriterFactory : public writer::WriterFactory {
  std::unique_ptr<writer::Writer> Build(const std::string &key) final;

  void DebugPrint(std::ostream &oss) const {
    std::string sep;

    oss << "FakeBufferWriterFactory Debug Print:" << std::endl;
    for (const auto &[name, content] : Files) {
      oss << sep;
      sep = "=======================================";

      oss << "*** File: " << name << std::endl;
      oss << content << std::endl;
    }
  }

  std::unordered_map<std::string, std::string> Files;
};

FakeBufferWriter::~FakeBufferWriter() {
  MyFactory->Files[Key] = Buffer();
}

std::unique_ptr<writer::Writer> FakeBufferWriterFactory::Build(
    const std::string &key) {
  return std::unique_ptr<writer::Writer>{new FakeBufferWriter{this, key}};
}

static rules::BuildPackageFactory SimpleProject() {
  rules::BuildPackageFactory factory;

  {
    auto package = factory.Package("library/base");
    auto base = new rules::CCLibrary(package, "base");
    base->Includes.push_back("base_inherit_include_directory");
    base->Defines.push_back("base_inherit_define_flag");
    base->Sources = {"base1.cpp", "base2.cpp", "base3.cpp"};
    base->CppFlags.push_back("-Dbase_only_defie_flag");
  }
  {
    auto package = factory.Package("library/memory");
    auto memory = new rules::CCLibrary(package, "memory");
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

struct NopExpander final : filesystem::FileNamePatternExpander {
  std::list<std::string> Expand(const std::string &pattern,
                                const common::AbsolutePath &path) final;
};

std::list<std::string> NopExpander::Expand(const std::string &pattern,
                                           const common::AbsolutePath &) {
  return std::list<std::string>{pattern};
}

TEST_CASE("Compile cc_library", "[compiler][makefile]") {
  filesystem::ProjectFileSystem project{
      common::AbsolutePath{"~/Projects/agora"},
      common::AbsolutePath{"~/Projects/agora/.build"},
  };
  FakeBufferWriterFactory writer_factory;
  auto compiler =
      CompilerFactory::Instance()->FindCompiler("Makefile.cc_library");
  NopExpander expander;

  auto simple_project = SimpleProject();

  SECTION("library/base:base") {
    auto rule = simple_project.Package("library/base")->Rules["base"].get();
    compiler->Compile(&project, &writer_factory, rule, &expander);

    writer_factory.DebugPrint(std::cout);

    REQUIRE(true);
  }

  SECTION("library/memory:memory") {
    auto rule = simple_project.Package("library/memory")->Rules["memory"].get();
    compiler->Compile(&project, &writer_factory, rule, &expander);

    writer_factory.DebugPrint(std::cout);

    REQUIRE(true);
  }
}

}  // namespace jk::core::compile::test

// vim: fdm=marker
