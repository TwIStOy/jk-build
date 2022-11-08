// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_rule.hh"

#include <future>
#include <memory>

#include "jk/core/models/session.hh"
#include "jk/utils/assert.hh"

namespace jk::core::models {

BuildRule::BuildRule(BuildPackage *package, std::string type_name,
                     RuleType type, std::string_view package_name,
                     utils::Kwargs kwargs)
    : Package(package),
      Base(std::make_unique<BuildRuleBase>(type_name, type, package_name,
                                           std::move(kwargs))) {
}

auto BuildRule::Prepare(core::models::Session *session) -> std::future<void> {
  return session->Executor->Push([this, session]() mutable {
    DoPrepare(session);
    prepared_ = true;
  });
}

void BuildRule::DoPrepare(core::models::Session *session) {
  WorkingFolder =
      session->Project->BuildRoot.Sub(*Base->FullQuotedQualifiedName);
}

const std::vector<std::string> &ExportedFiles(Session *session,
                                              std::string_view build_type) {
  (void)session;
  (void)build_type;
  static std::vector<std::string> tmp;
  return tmp;
}

}  // namespace jk::core::models
