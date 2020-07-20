// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/echo_color.hh"

#include <iostream>

#include "args.hxx"
#include "fmt/core.h"
#include "jk/utils/str.hh"

namespace jk {

#define COLOR(color) args::Flag color(color_group, #color, #color, {#color})

static const char *code_green = "\u001b[32m";
static const char *code_red = "\u001b[31m";
static const char *code_blue = "\u001b[34m";
static const char *code_rst = "\u001b[0m";
static const char *code_bold = "\u001b[1m";

void EchoColor(args::Subparser &parser) {
  args::ValueFlag<std::string> sw(parser, "SWITCH", "Color is open or not.",
                                  {"switch"}, args::Options::Required);
  args::Group color_group(parser,
                          "color validation:", args::Group::Validators::Xor);
  COLOR(green);
  COLOR(red);
  COLOR(blue);
  args::Flag bold(parser, "BLOD", "BOLD", {"bold"});

  args::ValueFlag<uint32_t> progress_total(
      parser, "TOTAL", "Max progress number", {"progress-total"},
      args::Options::Required);
  args::ValueFlag<uint32_t> progress_num(
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

  auto p = 0;
  if (args::get(progress_total) > 0) {
    p = args::get(progress_num) * 100 / args::get(progress_total);
  }

  std::cout << fmt::format(
                   "{}[{:3d}%] {}{}", code_st, p,
                   utils::JoinString(" ", std::begin(msg), std::end(msg)),
                   code_ed)
            << std::endl;
}

}  // namespace jk

// vim: fdm=marker

