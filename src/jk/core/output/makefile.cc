// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/output/makefile.hh"

#include <list>
#include <string>

#include "jk/common/flags.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/writer/writer.hh"
#include "jk/version.h"

namespace jk::core::output {

UnixMakefile::UnixMakefile(const std::string &filename) : filename_(filename) {
}

void UnixMakefile::DefineEnvironment(const std::string &key, std::string value,
                                     std::string comments) {
  Environments[key] =
      EnvironmentItem{key, std::move(value), std::move(comments)};
}

void UnixMakefile::Include(fs::path file_path, std::string comments,
                           bool fatal) {
  Includes.push_back(
      IncludeItem{std::move(file_path), std::move(comments), fatal});
}

void UnixMakefile::AddTarget(std::string target, std::list<std::string> deps,
                             std::list<std::string> statements,
                             std::string comments, bool phony) {
  Targets.push_back(TargetItem{std::move(target), std::move(deps),
                               std::move(statements), std::move(comments),
                               phony});
}

void UnixMakefile::DefineCommon(filesystem::ProjectFileSystem *project) {
  DefineEnvironment("SHELL", "/bin/bash",
                    "The shell in which to execute make rules.");

  DefineEnvironment("JK_COMMAND", "jk", "The command Jk executable.");

  DefineEnvironment("JK_SOURCE_DIR", project->ProjectRoot.Stringify(),
                    "The top-level source directory on which Jk was run.");

  DefineEnvironment("JK_BINARY_DIR", project->BuildRoot.Stringify(),
                    "The top-level build directory on which Jk was run.");

  DefineEnvironment("EQUALS", "=", "Escaping for special characters.");

  DefineEnvironment("PRINT", "jk echo_color");

  if (common::FLAGS_platform == common::Platform::k32) {
    DefineEnvironment("PLATFORM", "32");
  } else {
    DefineEnvironment("PLATFORM", "64");
  }
}

static const char *CommonHeader[] = {
    "# JK generated file: DO NOT EDIT!",
    "# Generated by \"Unix Makefiles\" Generator, JK Version " JK_VERSION};
static const char *Separator =
    "#========================================================================="
    "====";

void UnixMakefile::WriteComment(std::ostream &oss, const std::string &str) {
  if (str.length()) {
    oss << "# " << str << std::endl;
  }
}

std::string UnixMakefile::WriteToString() const {
  std::ostringstream oss;

  for (auto line : CommonHeader) {
    oss << line << std::endl;
  }
  oss << Separator << std::endl;

  WriteComment(oss, "Set environment variables for the build.");
  oss << std::endl;

  for (const auto &[key, env] : Environments) {
    WriteComment(oss, env.Comments);
    oss << fmt::format("{} = {}", env.Key, env.Value) << std::endl;
    oss << std::endl;
  }

  for (const auto &inc : Includes) {
    WriteComment(oss, inc.Comments);
    if (inc.Fatal) {
      oss << fmt::format("include {}", inc.Path.string()) << std::endl;
    } else {
      oss << fmt::format("-include {}", inc.Path.string()) << std::endl;
    }
  }

  for (const auto &tgt : Targets) {
    WriteComment(oss, tgt.Comments);
    oss << fmt::format("{}: {}", tgt.TargetName,
                       utils::JoinString(" ", std::begin(tgt.Dependencies),
                                         std::end(tgt.Dependencies)));
    oss << std::endl;
    for (const auto &stmt : tgt.Statements) {
      oss << fmt::format("\t{}", stmt) << std::endl;
    }

    if (tgt.Phony) {
      oss << fmt::format(".PHONY : {}", tgt.TargetName) << std::endl;
    }

    oss << std::endl;
  }

  return oss.str();
}

void UnixMakefile::Write(writer::Writer *writer) const {
  writer->Write(WriteToString());
  writer->Flush();
}

}  // namespace jk::core::output

