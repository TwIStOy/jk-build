// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/generators/makefile.hh"

#include <vector>

#include "absl/strings/ascii.h"
#include "jk/core/interfaces/writer.hh"
#include "jk/version.h"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/transform.hpp"

namespace jk::core::generators {

static const char *CommonHeader[] = {
    "# JK generated file: DO NOT EDIT!",
    "# Generated by \"Unix Makefiles\" Generator, JK Version " JK_VERSION};
static const char *Separator =
    "#========================================================================="
    "====";

Makefile::Makefile(common::AbsolutePath path,
                   std::vector<interfaces::WriterFactory *> writers)
    : path_(std::move(path)),
      writers_(writers | ranges::views::transform([](auto wf) {
                 return wf->Create();
               }) |
               ranges::to_vector) {
  for (auto &w : writers_) {
    w->open(path_);
  }

  for (const auto &line : CommonHeader) {
    for (auto &w : writers_) {
      w->write_line(line);
    }
  }

  print_sep();
  print_line();

  print_comment("default target");
  print_line(DEFAULT_TARGET, ":");

  print_line();

  print_comment("Delete rule output on recipe failure.");
  print_line(".DELETE_ON_ERROR:");
  print_sep();
  print_line();

  print_comment("Special targets provided by jk.");

  print_line(R"(
# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

# Suppress display of executed commands.
$(VERBOSE).SILENT:
  )");
  print_sep();
}

void Makefile::print_sep() {
  print_line(Separator);
}

Makefile &Makefile::Env(std::string_view key, std::string_view value,
                        std::string_view comment) {
  print_comment(comment);
  print_line(absl::AsciiStrToUpper(key), " = ", value);
  print_line();
  return *this;
}

Makefile &Makefile::Include(std::string_view filename, std::string_view comment,
                            bool fatal) {
  print_comment(comment);
  if (fatal) {
    print_line("include ", filename);
  } else {
    print_line("-include ", filename);
  }
  return *this;
}

}  // namespace jk::core::generators
