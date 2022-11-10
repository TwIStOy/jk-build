// Copyright (c) 2020 - present, Hawtian Wang (twistoy.wang@gmail.com)
//

#include "jk/impls/rules/cc_binary.hh"

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

}  // namespace jk::impls::rules
