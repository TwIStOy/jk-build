// Copyright (c) 2020 Hawtian Wang
//

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "jk/core/rules/build_rule.hh"
#include "jk/utils/stack.hh"

namespace jk {
namespace core {
namespace rules {

using BuildRuleMap =
    std::unordered_map<std::string, std::unique_ptr<BuildRule>>;
using BuildPackageMap =
    std::unordered_map<std::string, std::unique_ptr<BuildPackage>>;

struct BuildPackage {
  //! A package's name is its relative path from project root.
  std::string Name;

  //! All rules in this package.
  BuildRuleMap Rules;

  //! Initialize package. Note that a package can not be initialized twice, the
  //! second call will not take effect.
  void Initialize(utils::CollisionNameStack *stk);

 private:
  bool initialized_;
};

class BuildPackageFactory {
 public:
  BuildPackage *CreatePackage(const std::string &name);

 private:
  BuildPackageMap packages_;
};

}  // namespace rules
}  // namespace core
}  // namespace jk

