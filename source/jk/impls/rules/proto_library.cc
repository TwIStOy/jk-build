// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/proto_library.hh"

#include "absl/strings/ascii.h"

namespace jk::impls::rules {

ProtoLibrary::ProtoLibrary(core::models::BuildPackage *package,
                           utils::Kwargs kwargs, std::string type_name,
                           core::models::RuleType type)
    : CCLibrary(package, std::move(kwargs), std::move(type_name), type) {
  InherentFlags = {fmt::format(
      "-I${{{}_WORKING_FOLDER}}",
      absl::AsciiStrToUpper(Base->FullQuotedQualifiedNameWithoutVersion))};
}

void ProtoLibrary::DoPrepare(core::models::Session *session) {
  CCLibrary::DoPrepare(session);
}

}  // namespace jk::impls::rules
