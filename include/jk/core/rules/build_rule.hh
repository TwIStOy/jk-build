// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>
#include <unordered_map>

#include "jk/core/error.h"
#include "pybind11/pytypes.h"

namespace jk {
namespace core {
namespace rules {

struct BuildPackage;

/// BuildRule indicates a build-rule in process stage.
struct BuildRule {
  BuildRule(BuildPackage* package, std::string name);

  virtual ~BuildRule() = default;

  //! Return the rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  std::string FullQualifiedName() const;

  //! Check if this rule is *stable*. *Stable* means that the generated result
  //! and build result of this rule can be cached.
  virtual bool IsStable() const = 0;

  //! Extract fields from arguments
  virtual void ExtractFieldFromArguments(
      const std::unordered_map<std::string, pybind11::object>& kwargs) = 0;

  //! Which package where this build-rule is inside
  BuildPackage* Package;
  std::string Name;
  std::list<BuildRule*> Dependencies;
};

template <typename RuleType>
void NewRuleFromScript(
    BuildPackage* pkg,
    const std::unordered_map<std::string, pybind11::object>& kwargs) {
  // extract "name" from kwargs
  auto it = kwargs.find("name");
  if (it == kwargs.end()) {
    throw JKBuildError("expect field 'name' but not found");
  }
  if (it->second.get_type().is(pybind11::str().get_type())) {
    auto name = it->second.cast<std::string>();
    // construct a new rule
    new RuleType(pkg, name);
  } else {
    throw JKBuildError(
        "field 'name' expect str but {} found",
        pybind11::str(it->second.get_type()).cast<std::string>());
  }
}

}  // namespace rules
}  // namespace core
}  // namespace jk

