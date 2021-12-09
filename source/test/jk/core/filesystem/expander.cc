// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/expander.hh"

#include "fmt/core.h"

#define JK_TEST

#include <catch.hpp>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <list>
#include <random>

namespace jk::core::filesystem::testing {

namespace fs = std::filesystem;

TEST_CASE("DefaultExpanderTest", "[core][filesystem][expander]") {
  /*
   * kDefaultPatternExpander.Expand(const std::string &pattern,
   *                                const common::AbsolutePath &path);
   */
  std::random_device rd;

  auto temp_folder = fs::path("/tmp") / "jk-test";
  fs::create_directory(temp_folder);

  SECTION("no name") {
    for (auto i = 0u; i < 10; ++i) {
      auto filename = temp_folder / fmt::format("expander_test_{}", i);
      std::ofstream ofs(filename);
      ofs << "TMP";
    }
    for (auto i = 0u; i < 10; ++i) {
      auto filename = temp_folder / fmt::format("expander_test2_{}", i);
      std::ofstream ofs(filename);
      ofs << "TMP";
    }

    auto res = kDefaultPatternExpander.Expand(
        "expander_test_*", common::AbsolutePath{temp_folder});
    auto res2 = kDefaultPatternExpander.Expand(
        "expander_test2_*", common::AbsolutePath{temp_folder});
    auto res_all = kDefaultPatternExpander.Expand(
        "expander_test*", common::AbsolutePath{temp_folder});

    REQUIRE(res.size() == 10);
    REQUIRE(res2.size() == 10);
    REQUIRE(res_all.size() == 20);
  }
}

}  // namespace jk::core::filesystem::testing

// vim: fdm=marker
