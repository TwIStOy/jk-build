// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/core/models/build_rule.hh"

#include <future>

#include "jk/core/models/session.hh"
#include "jk/utils/assert.hh"

namespace jk::core::models {

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

}  // namespace jk::core::models
