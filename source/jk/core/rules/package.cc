// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/package.hh"

#include <list>
#include <memory>
#include <sstream>
#include <utility>

#include "jk/common/path.hh"
#include "jk/core/constant.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/script/script.hh"
#include "jk/utils/logging.hh"
#include "jk/utils/stack.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

static auto logger = utils::Logger("Package");

BuildPackage::BuildPackage(std::string name,  // {{{
                           common::ProjectRelativePath path)
    : Name(std::move(name)), Path(std::move(path)) {
  logger->debug(R"(New BuildPackage at "{}")", Path);
}  // }}}

void BuildPackage::Initialize(filesystem::JKProject *project) {
  if (init_state_ == InitializeState::kProcessing ||
      init_state_ == InitializeState::kDone) {
    return;
  }

  init_state_ = InitializeState::kProcessing;

  auto filename = Path.Path / "BUILD";
  auto interp = script::ScriptInterpreter::Instance();
  interp->EvalScript(project, this, filename.c_str());

  // Initialize done.
  init_state_ = InitializeState::kDone;
}

std::string BuildPackage::Stringify() const {
  return R"(BuildPackage(Name = {}, Rules = [], Path = {}))"_format(
      Name,
      utils::JoinString(", ", Rules.begin(), Rules.end(),
                        [](const auto &pr) {
                          return pr.first;
                        }),
      Path);
}

BuildPackage *BuildPackageFactory::Package(const std::string &name) {
  auto it = packages_.find(name);
  if (it == packages_.end()) {
    auto pkg =
        new BuildPackage(name, common::ProjectRelativePath{fs::path{name}});

    it = packages_
             .insert(std::make_pair(name, std::unique_ptr<BuildPackage>(pkg)))
             .first;
  }
  return it->second.get();
}

}  // namespace rules
}  // namespace core
}  // namespace jk
