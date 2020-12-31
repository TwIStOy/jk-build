// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/rules/cc_binary.hh"

#include <algorithm>
#include <iterator>
#include <list>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "jk/core/rules/build_rule.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/str.hh"

namespace jk::rules::cc {

static auto logger = utils::Logger("cc_binary");

std::vector<std::string> CCBinary::ResolveDependenciesAndLdFlags(
    core::filesystem::JKProject *project, const std::string &build_type) const {
  std::vector<std::string> res;

  res.insert(res.end(), std::begin(LdFlags), std::end(LdFlags));

  for (auto rule : DependenciesInOrder()) {
    if (rule == this) {
      continue;
    }

    auto exported_files = rule->ExportedFilesSimpleName(project, build_type);

    if (rule->Type.HasType(RuleTypeEnum::kExternal) &&
        exported_files.size() > 1) {
      // if external and multiple static libraries, order maybe wrong,
      // help them to ignore orders
      res.push_back("-Wl,--start-group");
      std::copy(std::begin(exported_files), std::end(exported_files),
                std::back_inserter(res));
      res.push_back("-Wl,--end-group");
    } else {
      std::copy(std::begin(exported_files), std::end(exported_files),
                std::back_inserter(res));
    }
    auto exported_ldflags = rule->ExportedLinkFlags();
    res.insert(res.end(), std::begin(exported_ldflags),
               std::end(exported_ldflags));
  }

  logger->info("Resolve dep and flags by depends in order: [{}]",
               utils::JoinString(", ", DependenciesInOrder(), [](auto p) {
                 return p->Stringify();
               }));
  logger->debug("Resolved dep and flags: [{}]",
                utils::JoinString(", ", std::begin(res), std::end(res),
                                  [](const std::string &s) {
                                    return "\"{}\""_format(s);
                                  }));

  return res;
}

}  // namespace jk::rules::cc

// vim: fdm=marker
