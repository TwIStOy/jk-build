// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <ostream>
#include <string>
#include <unordered_map>

#include "jk/core/builder/builder.hh"
#include "jk/core/filesystem/project.hh"

namespace jk {
namespace core {
namespace builder {

class MakefileBuilder {
 public:
  MakefileBuilder();

  void DefineEnvironment(const std::string &key, std::string value,
                         std::string comments = "");

  void Include(fs::path file_path, std::string comments = "",
               bool fatal = false);

  void AddTarget(std::string target, std::list<std::string> deps,
                 std::list<std::string> statements = {},
                 std::string Comments = "", bool phony = false);

  void WriteToFile(fs::path file) const;

 private:
  static void WriteComment(std::ostream &, const std::string &str);

 private:
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

}  // namespace builder
}  // namespace core
}  // namespace jk

