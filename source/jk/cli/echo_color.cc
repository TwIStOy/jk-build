// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/echo_color.hh"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "args.hxx"
#include "fmt/core.h"
#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::cli {

#define COLOR(color) args::Flag color(color_group, #color, #color, {#color})

static const char *code_red = "\u001b[31m";
static const char *code_green = "\u001b[32m";
static const char *code_yellow = "\u001b[33m";
static const char *code_blue = "\u001b[34m";
static const char *code_magenta = "\u001b[35m";
static const char *code_cyan = "\u001b[36m";
static const char *code_rst = "\u001b[0m";
static const char *code_bold = "\u001b[1m";

static std::string ProgressReport(const fs::path &progress_dir_root,
                                  const std::vector<uint32_t> &num) {
  auto progress = progress_dir_root / "Progress";
  common::AssumeFolder(progress);
  uint32_t total;
  {
    std::ifstream ifs((progress / "count.txt").string());
    if (ifs) {
      ifs >> total;
    } else {
      JK_THROW(core::JKBuildError("Could not read from progress file."));
    }
  }

  for (const auto &n : num) {
    auto file = progress / std::to_string(n);
    std::ofstream ofs(file.string());
    if (ofs) {
      ofs << "empty";
    }
  }

  auto count =
      common::GetNumberOfFilesInDirectory(common::AbsolutePath{progress});
  if (total > 0) {
    return fmt::format("[{:3d}%]", (count - 1) * 100 / total);
  }
  return "";
}

void EchoColor(args::Subparser &parser) {
  args::ValueFlag<std::string> sw(parser, "SWITCH", "Color is open or not.",
                                  {"switch"}, args::Options::Required);
  args::Group color_group(parser,
                          "color validation:", args::Group::Validators::Xor);
  COLOR(red);
  COLOR(green);
  COLOR(yellow);
  COLOR(blue);
  COLOR(magenta);
  COLOR(cyan);
  args::Flag bold(parser, "BOLD", "BOLD", {"bold"});

  args::ValueFlag<std::string> progress_dir(
      parser, "DIR", "Progress-dir", {"progress-dir"}, args::Options::Required);
  args::ValueFlag<std::string> progress_num(
      parser, "CURRENT", "Current progress number", {"progress-num"},
      args::Options::Required);
  args::PositionalList<std::string> message(parser, "MESSAGE", "Messages.");

  parser.Parse();

  std::string code_st;
  std::string code_ed;
  if (args::get(sw) != "off") {
    bool has = false;
    if (green) {
      code_st = code_green;
      has = true;
    }
    if (red) {
      code_st = code_red;
      has = true;
    }
    if (blue) {
      code_st = code_blue;
      has = true;
    }

    if (bold) {
      code_st += code_bold;
      has = true;
    }

    if (has) {
      code_ed = code_rst;
    }
  }

  auto msg = args::get(message);
  std::vector<uint32_t> num;
  std::istringstream iss(args::get(progress_num));
  std::string item;
  while (std::getline(iss, item, ',')) {
    std::istringstream inner(item);
    uint32_t x;
    inner >> x;
    num.push_back(x);
  }

  std::cout << fmt::format(
                   "{} {}{}{}",
                   ProgressReport(fs::path{args::get(progress_dir)}, num),
                   code_st,
                   utils::JoinString(" ", std::begin(msg), std::end(msg)),
                   code_ed)
            << std::endl;
}

}  // namespace jk::cli

// vim: fdm=marker

