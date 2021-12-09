// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/tools/include_finder.hh"

#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "jk/core/parser/combinator/convertor.hh"
#include "jk/core/parser/combinator/many.hh"
#include "jk/core/parser/combinator/or.hh"
#include "jk/core/parser/combinator/plus.hh"
#include "jk/core/parser/input_stream.hh"
#include "jk/core/parser/parser.hh"
#include "jk/core/parser/parsers.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc::tools {

void IncludeFinder::IncludeDirectory(core::filesystem::JKProject *project,
                                     const std::string &directory) {
  fs::path p(directory);

  if (p.is_absolute()) {
    include_directories_.push_back(common::AbsolutePath(std::move(p)));
  } else {
    common::ProjectRelativePath rp(std::move(p));
    include_directories_.push_back(project->ProjectRoot.Sub(rp.Path));
  }
}

void IncludeFinder::ParseIncludeDirectoryFlags(
    core::filesystem::JKProject *project, const std::string &flag) {
  if (utils::StringStartsWith(flag, "-I")) {
    IncludeDirectory(project, flag.substr(2));
  }
}

// Include Line Matcher {{{
static auto empty_ch = core::parser::MakeCharPredict([](char ch) {
  return std::isspace(ch);
});
static auto include_keyword = core::parser::MakeStringEq("#include");
static auto espaced_char = (core::parser::MakeCharEq('\\') +
                            core::parser::MakeCharAny()) >>
                           [](std::tuple<char, char> r) -> char {
  return std::get<1>(r);
};
static auto string_ch = core::parser::MakeCharPredict([](char c) {
                          return c != '\\' && c != '"' && c != '>';
                        }) |
                        espaced_char;
static auto string_literal =
    (core::parser::MakeCharEq('"') + core::parser::Many(string_ch) +
     core::parser::MakeCharEq('"')) >>
    [](const std::tuple<char, const std::vector<char> &, char> &rp) {
      std::string res;
      auto &body = std::get<1>(rp);
      std::copy(std::begin(body), std::end(body), std::back_inserter(res));
      return res;
    };
static auto string_literal2 =
    (core::parser::MakeCharEq('<') + core::parser::Many(string_ch) +
     core::parser::MakeCharEq('>')) >>
    [](const std::tuple<char, const std::vector<char> &, char> &rp) {
      std::string res;
      auto &body = std::get<1>(rp);
      std::copy(std::begin(body), std::end(body), std::back_inserter(res));
      return res;
    };

static auto include_stmt =
    (core::parser::Many(empty_ch) + include_keyword +
     core::parser::Many(empty_ch) + (string_literal | string_literal2)) >>
    [](const std::tuple<std::vector<char>, std::string_view, std::vector<char>,
                        std::string> &rp) {
      return std::get<3>(rp);
    };
// }}}

std::optional<std::string> IncludeFinder::ParseIncludeLine(
    std::string_view line) {
  auto stream = core::parser::InputStream{line};
  auto res = include_stmt(stream);
  if (res.Success()) {
    return std::move(res).Result();
  }
  return {};
}

std::vector<common::AbsolutePath> IncludeFinder::Headers(
    const common::AbsolutePath &ap) {
  // Read all lines from file with 'filename'
  std::ifstream ifs(ap.Path);
  if (!ifs) {
    return {};
  }

  std::vector<common::AbsolutePath> headers;
  std::string line;

  while (std::getline(ifs, line)) {
    auto stream = core::parser::InputStream{line};
    auto res = include_stmt(stream);
    if (res.Success()) {
      auto name = res.Result();
      if (auto p = ResolveFileName(name); p) {
        headers.push_back(p.value());
      }
    }
  }

  return headers;
}

std::optional<common::AbsolutePath> IncludeFinder::ResolveFileName(
    const std::string &filename) {
  for (const auto &base : include_directories_) {
    auto check_path = base.Sub(filename);
    if (fs::exists(check_path.Path)) {
      return check_path;
    }
  }
  return {};
}

}  // namespace jk::rules::cc::tools

// vim: fdm=marker
