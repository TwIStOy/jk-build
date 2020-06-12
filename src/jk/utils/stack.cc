// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/stack.hh"

#include <sstream>

namespace jk {
namespace utils {

void CollisionNameStack::Pop() {
  if (ordered_names_.size()) {
    auto name = ordered_names_.back();
    ordered_names_.pop_back();
    visited_.erase(name);
  }
}

bool CollisionNameStack::Push(const std::string &name,
                              std::list<std::string> *stk) {
  auto it = visited_.find(name);
  if (it != visited_.end()) {
    if (stk != nullptr) {
      stk->clear();
      for (auto pos = it->second; pos != ordered_names_.end(); ++pos) {
        stk->push_back(*pos);
      }
    }

    return false;
  }

  auto pit = ordered_names_.insert(ordered_names_.end(), name);
  visited_[name] = pit;

  return true;
}

void CollisionNameStack::Clear() {
  ordered_names_.clear();
  visited_.clear();
}

std::string CollisionNameStack::DumpStack() const {
  std::ostringstream oss;
  bool first = true;
  for (auto name : ordered_names_) {
    if (first) {
      first = false;
      oss << "    ";
    } else {
      oss << " -> ";
    }
    oss << name << "\n";
  }
  return oss.str();
}

}  // namespace utils
}  // namespace jk

