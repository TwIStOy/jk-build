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

  //! This field MUST be used after `StartDependenciesConstruct` has been
  //! invoked.
  std::vector<BuildRule *> Dependencies;

  std::vector<std::string> ExportedLinkFlags;

  std::vector<std::pair<std::string, std::string>> ExportedEnvironmentVars;

  std::vector<std::string> Artifacts;

  common::AbsolutePath WorkingFolder;

  /*
   * //! Returns the absolute paths of what will this build-rule generated. All
   * //! rules depend on myself, will automatically depend on all my artifacts.
   * virtual const std::vector<common::AbsolutePath> &Artifacts(
   *     Session *session) const = 0;
   */

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
