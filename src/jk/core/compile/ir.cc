// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/compile/ir.hh"

namespace jk::core::compile::ir {

void Environment::NewVar(std::string key, std::string value,
                         std::string comment) {
  Vars[key].reset(new DefinedVariable{std::move(key), std::move(value),
                                      std::move(comment)});
}

DefinedVariable *Environment::Var(const std::string &name) const {
  auto it = Vars.find(name);
  if (it == Vars.end()) {
    return nullptr;
  }
  return it->second.get();
}

DefinedVariable *IR::Var(const std::string &ns, const std::string &name) const {
  auto it = Environments.find(ns);
  if (it == Environments.end()) {
    return nullptr;
  }
  return it->second.Var(name);
}

}  // namespace jk::core::compile::ir

// vim: fdm=marker

