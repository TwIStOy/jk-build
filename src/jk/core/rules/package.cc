// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/package.hh"

#include <list>
#include <memory>
#include <sstream>
#include <utility>

#include "jk/core/error.h"

namespace jk {
namespace core {
namespace rules {

void PackageNameStack::Pop() {
  if (packages_.size()) {
    auto pkg = packages_.back();
    packages_.pop_back();
    visited_.erase(pkg->Name);
  }
}

bool PackageNameStack::Push(BuildPackage* pkg, std::list<BuildPackage*>* stk) {
  auto it = visited_.find(pkg->Name);
  if (it != visited_.end()) {
    if (stk != nullptr) {
      stk->clear();
      for (auto pos = it->second; pos != packages_.end(); ++pos) {
        stk->push_back(*pos);
      }
    }

    return false;
  }

  auto pit = packages_.insert(packages_.end(), pkg);
  visited_[pkg->Name] = pit;

  return true;
}

void PackageNameStack::Clear() {
  packages_.clear();
  visited_.clear();
}

std::string PackageNameStack::DumpStack() const {
  std::ostringstream oss;
  bool first = true;
  for (auto pkg : packages_) {
    if (first) {
      first = false;
      oss << "    ";
    } else {
      oss << " -> ";
    }
    oss << pkg->Name << "\n";
  }
  return oss.str();
}

void BuildPackage::Initialize(const std::string_view& filename,
                              PackageNameStack* stk) {
  if (initialized_) {
    return;
  }

  if (stk != nullptr) {
    if (!stk->Push(this)) {
      throw JKBuildError(
          "Initialize pacakged {} failed, this package has been initialized "
          "before in ths stage. It may be a circle. {}",
          Name, stk->DumpStack());
    }
  }

  // TODO(hawtian): load python script

  // Initialize done.
  initialized_ = true;
}

BuildPackage* BuildPackageFactory::CreatePackage(const std::string& name) {
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
