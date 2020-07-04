// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/package.hh"

#include <list>
#include <memory>
#include <sstream>
#include <utility>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/core/script/script.hh"
#include "jk/utils/stack.hh"
#include "jk/utils/str.hh"

namespace jk {
namespace core {
namespace rules {

void BuildPackage::Initialize(utils::CollisionNameStack *stk) {
  if (initialized_) {
    return;
  }

  if (stk != nullptr) {
    if (!stk->Push(this->Name)) {
      throw JKBuildError(
          "Initialize pacakged {} failed, this package has been initialized "
          "before in ths stage. It may be a circle. {}",
          Name, stk->DumpStack());
    }
  }

  auto filename = Path.Path / "BUILD";
  auto interp = script::ScriptInterpreter::Instance();
  interp->EvalScript(this, filename.c_str());

  // Initialize done.
  initialized_ = true;

  stk->Pop();
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
