// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/rules/build_rule.hh"

#include <sstream>
#include <string>

#include "fmt/format.h"
#include "jk/core/rules/package.hh"

namespace jk {
namespace core {
namespace rules {

BuildRule::BuildRule(BuildPackage* package, std::string name)
    : Package(package), Name(std::move(name)) {
  package->Rules[Name].reset(this);
}

std::string BuildRule::FullQualifiedName() const {
  return fmt::format("{}/{}", Package->Name, Name);
}

}  // namespace rules
}  // namespace core
}  // namespace jk

