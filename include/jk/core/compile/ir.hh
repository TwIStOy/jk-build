// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

#include "boost/variant.hpp"
#include "jk/core/filesystem/project.hh"

namespace jk::core::compile::ir {

struct DefinedVariable {
  std::string Key;
  std::string Value;
  std::string Comment;
};

struct Environment {
  std::unordered_map<std::string, std::unique_ptr<DefinedVariable>> Vars;

  void NewVar(std::string key, std::string value, std::string comment = "");

  DefinedVariable *Var(const std::string &name) const;
};

using StmtElement = boost::variant<std::string, DefinedVariable *>;

struct Statement {
  std::string Hint;
  std::list<StmtElement> Elements;
};

struct Target {
  std::string Name;
  std::list<Statement> Statements;
  std::list<std::string> Dependencies;
  std::string Comments;
  bool Phony;
};

struct IncludeItem {
  std::string Tag;
  std::string Comments;
  bool Fatal;
};

struct Page {
  std::list<IncludeItem> Includes;
  std::list<Target> Targets;
};

struct IR {
  DefinedVariable *Var(const std::string &ns, const std::string &name) const;

  // std::list<Statement> Statements;

  std::unordered_map<std::string, Environment> Environments;
  std::unordered_map<std::string, Page> Pages;
};

}  // namespace jk::core::compile::ir

// vim: fdm=marker

