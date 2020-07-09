// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>

#include "jk/core/filesystem/project.hh"
#include "jk/core/writer/writer.hh"

namespace jk::core::output {

struct UnixMakefile {
  UnixMakefile(const std::string &filename);

  void DefineCommon(filesystem::ProjectFileSystem *project);

  void DefineEnvironment(const std::string &key, std::string value,
                         std::string comments = "");

  void Include(fs::path file_path, std::string comments = "",
               bool fatal = false);

  void AddTarget(std::string target, std::list<std::string> deps,
                 std::list<std::string> statements = {},
                 std::string Comments = "", bool phony = false);

  void Write(writer::Writer *writer) const;

  std::string WriteToString() const;

 private:
  static void WriteComment(std::ostream &, const std::string &str);

 private:
  std::string filename_;
  struct EnvironmentItem {
    std::string Key;
    std::string Value;
    std::string Comments;
  };

  struct IncludeItem {
    fs::path Path;
    std::string Comments;
    bool Fatal;
  };

  struct TargetItem {
    std::string TargetName;
    std::list<std::string> Dependencies;
    std::list<std::string> Statements;
    std::string Comments;
    bool Phony;
  };

  std::unordered_map<std::string, EnvironmentItem> environments_;
  std::list<IncludeItem> includes_;
  std::list<TargetItem> targets_;
};

}  // namespace jk::core::output