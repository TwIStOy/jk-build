// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <initializer_list>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "jk/common/path.hh"
#include "jk/core/error.h"
#include "jk/utils/kwargs.hh"
#include "jk/utils/stack.hh"
#include "pybind11/embed.h"
#include "pybind11/eval.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

namespace jk {
namespace core {
namespace rules {

struct BuildPackage;
class BuildPackageFactory;

// clang-format off
enum class RuleTypeEnum : uint8_t {
  kLibrary = 1 << 0,
  kBinary  = 1 << 1,
  kTest    = 1 << 2,
};
// clang-format on

#define TYPE_SET_GETTER(type)                              \
  inline bool Is##type() const {                           \
    return HasType(RuleTypeEnum::k##type);                 \
  }                                                        \
  inline void Set##type() {                                \
    value_ |= static_cast<uint8_t>(RuleTypeEnum::k##type); \
  }

struct RuleType {
 public:
  inline bool HasType(RuleTypeEnum tp) const {
    return value_ & static_cast<uint8_t>(tp);
  }

  inline void SetType(RuleTypeEnum tp) {
    value_ |= static_cast<uint8_t>(tp);
  }

  TYPE_SET_GETTER(Library);
  TYPE_SET_GETTER(Binary);
  TYPE_SET_GETTER(Test);

 private:
  uint8_t value_;
};

#undef TYPE_SET_GETTER

/// BuildRule indicates a build-rule in process stage.
struct BuildRule {
  BuildRule(BuildPackage *package, std::string name,
            std::initializer_list<RuleTypeEnum> types,
            std::string_view type_name);

  virtual ~BuildRule() = default;

  //! Return the rule's full qualifed name. This named will automatically be
  //! converted into the rules folder name, just like cmake does.
  std::string FullQualifiedName() const;

  //! After dependencies has been built, this filed will be available
  //! pstk: package name stack
  //! rstk: rule name stack
  void BuildDependencies(BuildPackageFactory *factory,
                         utils::CollisionNameStack *pstk,
                         utils::CollisionNameStack *rstk);

  //! Downcast
  template<typename T>
  T *Downcast() {
    return dynamic_cast<T *>(this);
  }

  //! Downcast
  template<typename T>
  T *const Downcast() const {
    return dynamic_cast<T *const>(this);
  }

  //! Check if this rule is *stable*. *Stable* means that the generated result
  //! and build result of this rule can be cached.
  virtual bool IsStable() const = 0;

  //! Returns exported names. Other rules which depend on this, will
  //! automatically depend on all exported files.
  virtual std::vector<std::string> ExportedFileName() const = 0;

  //! Extract fields from arguments
  virtual void ExtractFieldFromArguments(const utils::Kwargs &kwargs);

  //! Which package where this build-rule is inside
  BuildPackage *Package;
  std::string Name;
  std::list<BuildRule *> Dependencies;
  const RuleType Type;
  const std::string_view TypeName;

 private:
  bool dependencies_has_built_{false};
  std::vector<std::string> dependencies_str_;
};

template<typename RuleType>
void NewRuleFromScript(
    BuildPackage *pkg,
    const std::unordered_map<std::string, pybind11::object> &kwargs) {
  // extract "name" from kwargs
  auto it = kwargs.find("name");
  if (it == kwargs.end()) {
    throw JKBuildError("expect field 'name' but not found");
  }
  if (it->second.get_type().is(pybind11::str().get_type())) {
    auto name = it->second.cast<std::string>();
    // construct a new rule
    auto rule = new RuleType(pkg, name);
    rule->ExtractFieldFromArguments(utils::Kwargs{kwargs});
  } else {
    throw JKBuildError(
        "field 'name' expect str but {} found",
        pybind11::str(it->second.get_type()).cast<std::string>());
  }
}

}  // namespace rules
}  // namespace core
}  // namespace jk

