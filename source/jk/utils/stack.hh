// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <list>
#include <string>
#include <unordered_map>

#include "jk/utils/str.hh"

namespace jk {
namespace utils {

class CollisionNameStack final : public Stringifiable {
 public:
  struct ScopedElement {
    ScopedElement();
    ScopedElement(ScopedElement &&);
    ~ScopedElement();

    CollisionNameStack *stk = nullptr;
  };

  //! Pop the last name from stack. It is safe if the stack is empty.
  void Pop();

  //! Try to push a name into stack.
  //! If this name has already been in this stack, return false and if
  //! **stk** is not nullptr, the packages from first occur to top will be
  //! placed into **stk**(from bottom to top).
  bool Push(const std::string &name, std::list<std::string> *stk = nullptr);

  //! Clear stack.
  void Clear();

  //! Dump all elements in this stack into a string, formatted:
  //!   {NAME} -> {NAME} -> ...
  std::string DumpStack() const;

  //! RAII Element
  ScopedElement ScopedPush(const std::string &,
                           std::list<std::string> *stk = nullptr);

  // inherited from |Stringifiable|
  std::string Stringify() const final;

 private:
  std::list<std::string> ordered_names_;
  std::unordered_map<std::string, std::list<std::string>::iterator> visited_;
};

}  // namespace utils
}  // namespace jk
