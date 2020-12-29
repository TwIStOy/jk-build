// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
#include <unordered_map>
#include <vector>

#include "jk/rules/external/rules/external_library.hh"

namespace jk::rules::external {

class CMakeLibrary : public ExternalLibrary {
 public:
  CMakeLibrary(core::rules::BuildPackage *package, std::string name);

  void ExtractFieldFromArguments(const utils::Kwargs &kwargs) override;

  std::vector<std::string> ExportedLinkFlags() const override;

  // --- Fields Start ---
  std::unordered_map<std::string, std::string> CMakeVariable;
  uint32_t JobNumber = 1;
  // ---- Fields End ----
};

}  // namespace jk::rules::external

// vim: fdm=marker
