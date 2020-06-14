// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/package.hh"

#include <list>
#include <memory>
#include <sstream>
#include <utility>

#include "jk/core/error.h"
#include "jk/core/filesystem/project.hh"
#include "jk/utils/stack.hh"

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

  // TODO(hawtian): load python script
  auto filename = filesystem::ProjectRoot() / "BUILD";

  // Initialize done.
  initialized_ = true;

  stk->Pop();
}

BuildPackage *BuildPackageFactory::CreatePackage(const std::string &name) {
  auto it = packages_.find(name);
  if (it == packages_.end()) {
    it = packages_
             .insert(std::make_pair(
                 name, std::unique_ptr<BuildPackage>(new BuildPackage)))
             .first;
  }
  return it->second.get();
}

}  // namespace rules
}  // namespace core
}  // namespace jk
