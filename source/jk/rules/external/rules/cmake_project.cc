// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/external/rules/cmake_project.hh"

#include <unordered_map>
#include <vector>

#include "jk/common/flags.hh"
#include "jk/core/error.h"
#include "jk/rules/external/rules/external_library.hh"
#include "jk/utils/logging.hh"

namespace jk::rules::external {

CMakeLibrary::CMakeLibrary(core::rules::BuildPackage *package, std::string name)
    : ExternalLibrary(package, name, "cmake_library") {
}

void CMakeLibrary::ExtractFieldFromArguments(const utils::Kwargs &kwargs) {
  ExternalLibrary::ExtractFieldFromArguments(kwargs);
  auto empty_list = boost::make_optional<std::vector<std::string>>({});

  {
    auto it = kwargs.Find("var");
    if (it != kwargs.End()) {
      if (!pybind11::isinstance<pybind11::dict()>(it->second)) {
        JK_THROW(core::JKBuildError("field 'var' expect type dict"));
      }

      CMakeVariable =
          it->second.cast<std::unordered_map<std::string, std::string>>();
    }
  }

  {
    auto it = kwargs.Find("job");
    if (it != kwargs.End()) {
      if (!pybind11::isinstance<pybind11::int_()>(it->second)) {
        JK_THROW(core::JKBuildError("field 'define' expect type int"));
      }

      JobNumber = it->second.cast<uint32_t>();
    }
  }

  do {
    auto it = kwargs.Find("export");
    if (it == kwargs.End()) {
      JK_THROW(core::JKBuildError("expect field '{}' but not found", "export"));
    }
    if (pybind11::isinstance<pybind11::str()>(it->second)) {
      Libraries = std::vector<std::string>{it->second.cast<std::string>()};
      break;
    }
    if (pybind11::isinstance<pybind11::list()>(it->second)) {
      Libraries =
          std::vector<std::string>{it->second.cast<utils::Kwargs::ListType>()};
      break;
    }

    JK_THROW(
        core::JKBuildError("field '{}' expect type list or str", "export"));
  } while (0);
}

std::vector<std::string> CMakeLibrary::ExportedLinkFlags() const {
  return LdFlags;
}

}  // namespace jk::rules::external

// vim: fdm=marker
