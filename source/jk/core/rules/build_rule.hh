// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <concepts>
#include <functional>
#include <initializer_list>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "jk/common/counter.hh"
#include "jk/common/path.hh"
#include "jk/core/constant.hh"
#include "jk/core/error.h"
#include "jk/core/filesystem/expander.hh"
#include "jk/core/filesystem/project.hh"
#include "jk/core/rules/build_rule_base.hh"
#include "jk/core/rules/rule_type.hh"
#include "jk/utils/kwargs.hh"
#include "jk/utils/lazy_eval.hh"
#include "jk/utils/stack.hh"
#include "jk/utils/str.hh"
#include "nlohmann/json.hpp"
#include "pybind11/embed.h"
#include "pybind11/eval.h"
#include "pybind11/pybind11.h"
#include "pybind11/pytypes.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"

namespace jk {

using nlohmann::json;

namespace core {
namespace rules {

struct BuildPackage;
class BuildPackageFactory;

/// BuildRule indicates a build-rule in process stage.
struct BuildRule : public BuildRuleBase {
  template<typename T>
  using lazy_t = utils::LazyEvaluatedValue<T>;

  BuildRule(BuildPackage *package, std::string name,
            std::initializer_list<RuleTypeEnum> types,
            std::string_view type_name);

  virtual ~BuildRule() = default;

  //! NOTE(hawtian): MUST invoke `Prepare` before all operations
  void Prepare(filesystem::JKProject *project, BuildPackageFactory *factory);
  bool Prepared() const;

  //! Return the rule's full qualifed target name.
  std::string FullQualifiedTarget(const std::string &output = "") const;

  lazy_t<uint32_t> SCC_ID;

  //! Downcast
  template<typename T>
  T *Downcast() {
    return static_cast<T *>(this);
  }

  //! Downcast
  template<typename T>
  T *const Downcast() const {
    return static_cast<T *const>(this);
  }

  //! Returns exported names. Other rules which depend on this, will
  //! automatically depend on all exported files.
  virtual std::vector<std::string> ExportedFilesSimpleName(
      filesystem::JKProject *project, const std::string &build_type) const = 0;

  //! Return exported link flags. Other rules which depend on this, will
  //! automatically add these flags into its link flags.
  virtual std::vector<std::string> ExportedLinkFlags() const = 0;

  //! Return exported environment variables, that its dependencies can be used.
  //!
  //! All variables start with the rule's full qualifed name. eg:
  //! <Rule, 'third_party/protobuf'>, variable: "protoc" =>
  //!   THIRD_PARTY_PROTOBUF_PROTOC
  virtual const std::vector<std::pair<std::string, std::string>>
      &ExportedEnvironmentVar(filesystem::JKProject *project) const;

  //! Extract fields from arguments
  virtual void ExtractFieldFromArguments(const utils::Kwargs &kwargs);

  //! Extract all deps after sorting.
  //! The result list assumes that a rule's dependencies must be after its first
  //! occurrences.
  std::list<BuildRule const *> DependenciesInOrder();

  std::list<BuildRule const *> DependenciesAlwaysBehind();

  //! Return working folder based on `build_root`
  common::AbsolutePath WorkingFolder(
      const common::AbsolutePath &build_root) const;

  //! Execute function recursively.
  template<typename Func>
    requires std::invocable<Func, jk::core::rules::BuildRule *>
  void RecursiveExecute(Func func,
                        std::unordered_set<std::string> *recorder = nullptr) {
    static auto logger = utils::Logger("rule");
    if (recorder) {
      if (auto it = recorder->find(FullQualifiedName());
          it != recorder->end()) {
        return;
      }
      recorder->insert(FullQualifiedName());
    }

    for (auto it : Dependencies) {
      it->RecursiveExecute(func, recorder);
    }

    func(this);
  }

  //! Allocate a new number if key not exists.
  uint32_t KeyNumber(const std::string &key);

  //! Return all allocated key numbers.
  std::vector<uint32_t> KeyNumbers() const;

  //! Which package where this build-rule is inside
  BuildPackage *Package;
  std::list<BuildRule *> Dependencies;
  const RuleType Type;
  const std::string_view TypeName;

  const std::string &Stringify() const;

 private:
  InitializeState dependencies_built_state_{InitializeState::kStart};

  common::CountableSteps steps_;

#ifdef JK_TEST
 public:
#endif
  std::vector<std::string> dependencies_str_;
  mutable boost::optional<std::list<BuildRule const *>> deps_sorted_list_;

  lazy_t<std::string> stringify_value_;
  mutable std::optional<std::list<BuildRule const *>> deps_always_behind_list_;
};

//! Create a **BuildRule** instance in *pkg* with *kwags".
//! The new rule will automatically register into its package.
template<typename RuleType>
void NewRuleFromScript(
    BuildPackage *pkg,
    const std::unordered_map<std::string, pybind11::object> &kwargs) {
  // extract "name" from kwargs
  auto it = kwargs.find("name");
  if (it == kwargs.end()) {
    JK_THROW(JKBuildError("expect field 'name' but not found"));
  }
  if (pybind11::isinstance<pybind11::str>(it->second)) {
    auto name = it->second.cast<std::string>();
    // construct a new rule
    auto rule = new RuleType(pkg, name);
    rule->ExtractFieldFromArguments(utils::Kwargs{kwargs});
  } else {
    JK_THROW(JKBuildError(
        "field 'name' expect {} but {} found",
        pybind11::str(pybind11::str().get_type()).cast<std::string>(),
        pybind11::str(it->second.get_type()).cast<std::string>()));
  }
}

}  // namespace rules
}  // namespace core
}  // namespace jk
