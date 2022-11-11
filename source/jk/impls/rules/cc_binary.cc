// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_binary.hh"

#include <variant>

#include "jk/core/error.h"
#include "jk/utils/logging.hh"

namespace jk::impls::rules {

CCBinary::CCBinary(core::models::BuildPackage *package, utils::Kwargs kwargs,
                   std::string type_name, core::models::RuleType type)
    : CCLibrary(package, std::move(kwargs), std::move(type_name), type) {
}

auto CCBinary::ExportedFiles(core::models::Session *session,
                             std::string_view build_type)
    -> const std::vector<std::string> & {
  _binary_tmp_file = {WorkingFolder.Sub(build_type, Base->Name).Stringify()};
  return _binary_tmp_file;
}

#define ADD_DEPS_FIELD "add_deps"

auto CCBinary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) -> void {
  CCLibrary::ExtractFieldFromArguments(kwargs);

  // addtional
  // add_deps = {
  //   "r0": ["r1", "r2"]
  // }
  // TODO(hawtian): update field name
  auto it = kwargs.Find(ADD_DEPS_FIELD);
  if (it != kwargs.End()) {
    const auto &add_deps = it->second;
    if (add_deps.value.index() != 2) {
      JK_THROW(
          core::JKBuildError("field '{}' expect type dict", ADD_DEPS_FIELD));
    }

    for (const auto &[r, deps] : std::get<2>(add_deps.value)) {
      if (deps->value.index() == 0) {
        RawAddtionalDependencies[r].push_back(std::get<0>(deps->value));
      } else if (deps->value.index() == 1) {
        for (const auto &dep : std::get<1>(deps->value)) {
          if (dep->value.index() != 0) {
            JK_THROW(core::JKBuildError("field '{}' value expect str",
                                        ADD_DEPS_FIELD));
          }
          RawAddtionalDependencies[r].push_back(std::get<0>(dep->value));
        }
      } else {
        JK_THROW(core::JKBuildError("field '{}' value expect str or list",
                                    ADD_DEPS_FIELD));
      }
    }
  }
}

}  // namespace jk::impls::rules
