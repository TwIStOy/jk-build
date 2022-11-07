// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#pragma once  // NOLINT(build/header_guard)

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/models/build_rule_base.hh"
#include "jk/utils/kwargs.hh"

namespace jk::core::models {

class BuildPackage;
class Session;

class BuildRule {
 public:
  BuildRule(BuildPackage *package, std::string type_name, RuleType type,
            std::string_view package_name, utils::Kwargs kwargs);

  //! Which package this build-rule inside
  BuildPackage *Package;

  //! Basic fields parsed from kwargs
  std::unique_ptr<BuildRuleBase> Base;

  std::future<void> StartDependenciesConstruct();

  std::future<void> Prepare(core::models::Session *session);

  //! Check if the build-rule is prepared.
  bool Prepared() const;

  std::vector<BuildRule *> Dependencies;

  std::vector<std::string> ExportedLinkFlags;

  std::vector<std::pair<std::string, std::string>> ExportedEnvironmentVars;

  //! Returns the absolute paths of what will this build-rule generated without
  //! build_type specifyed
  std::vector<std::string> Artifacts;

  const std::vector<std::string> &ExportedFiles(Session *session,
                                                std::string_view build_type);

  common::AbsolutePath WorkingFolder;

  //! All steps in this rule, this field will only be used in makefile
  //! generator.
  common::CountableSteps Steps;

  uint32_t _scc_id;

 protected:
  //! Extract and parse fields from rule-function's arguments. Every derived
  //! types should invoke its base-class' `ExtractFieldFromArguments`.
  virtual void ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  }

  virtual void DoPrepare(core::models::Session *session);

  bool prepared_{false};
};

inline bool BuildRule::Prepared() const {
  return prepared_;
}

}  // namespace jk::core::models
