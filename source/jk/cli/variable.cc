// Copyright (c) 2020 Hawtian Wang
//

#include "jk/cli/variable.hh"

namespace jk::cli {

void VariableGroup::Notify() {
  for (auto &f : callbacks_) {
    f();
  }
}

}  // namespace jk::cli

// vim: fdm=marker

