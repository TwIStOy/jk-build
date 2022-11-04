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
  //! Which package this build-rule inside
  BuildPackage *Package;

  //! Basic fields parsed from kwargs
  std::unique_ptr<BuildRuleBase> Base;

  std::future<void> StartPrepare(core::models::Session *session);

  bool Prepared() const;

  /*
   * //! Returns the absolute paths of what will this build-rule generated. All
   * //! rules depend on myself, will automatically depend on all my artifacts.
   * virtual const std::vector<common::AbsolutePath> &Artifacts(
   *     Session *session) const = 0;
   */

 protected:
  //! Extract and parse fields from rule-function's arguments. Every derived
  //! types should invoke its base-class' `ExtractFieldFromArguments`.
  virtual void ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  }

  virtual void Prepare(core::models::Session *session) = 0;

  bool prepared_{false};

 private:
  //! All steps in this rule, this field will only be used in makefile
  //! generator.
  common::CountableSteps steps_;
};

inline bool BuildRule::Prepared() const {
  return prepared_;
}

}  // namespace jk::core::models
