// Copyright (c) 2020 Hawtian Wang
//

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "jk/core/rules/build_rule.hh"

namespace jk {
namespace core {
namespace rules {

using BuildRuleMap =
    std::unordered_map<std::string, std::unique_ptr<BuildRule>>;
using BuildPackageMap =
    std::unordered_map<std::string, std::unique_ptr<BuildPackage>>;

class PackageNameStack {
 public:
  //! Pop the last package from stack. It is safe if the stack is empty.
  void Pop();

  //! Try to push a package into stack.
  //! If this package has already been in this stack, return false and if
  //! **stk** is not nullptr, the packages from first occur to top will be
  //! placed into **stk**(from bottom to top).
  bool Push(BuildPackage* pkg, std::list<BuildPackage*>* stk = nullptr);

  //! Clear stack.
  void Clear();

  //! Dump all elements in this stack into a string, formatted:
  //!   {PKG} -> {PKG} -> ...
  std::string DumpStack() const;

 private:
  std::list<BuildPackage*> packages_;
  std::unordered_map<std::string, std::list<BuildPackage*>::iterator> visited_;
};

struct BuildPackage {
  //! A package's name is its relative path from project root.
  std::string Name;

  //! All rules in this package.
  BuildRuleMap Rules;

  //! Initialize package. Note that a package can not be initialized twice, the
  //! second call will not take effect.
  void Initialize(const std::string_view& filename, PackageNameStack* stk);

 private:
  bool initialized_;
};

class BuildPackageFactory {
 public:
  BuildPackage* CreatePackage(const std::string& name);

 private:
  BuildPackageMap packages_;
};

}  // namespace rules
}  // namespace core
}  // namespace jk

