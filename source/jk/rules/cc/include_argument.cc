// Copyright (c) 2020 Hawtian Wang
//

#include "jk/rules/cc/include_argument.hh"

#include <variant>

#include "jk/utils/cpp_features.hh"

namespace jk::rules::cc {

IncludeArgument::IncludeArgument(std::string trivial) : data_(trivial) {
}

IncludeArgument::IncludeArgument(Placehoder p) : data_(p) {
}

std::string IncludeArgument::Stringify() const {
  return fmt::format(
      "IncArg({})",
      std::visit(utils::Overloaded{[](const std::string &x) -> std::string {
                                     return x;
                                   },
                                   [](Placehoder p) -> std::string {
                                     switch (p) {
                                       case Placehoder::WorkingFolder:
                                         return "Placehoder:WorkingFolder";
                                     }
                                     return "Placehoder:unknown";
                                   }},
                 data_));
}

}  // namespace jk::rules::cc

// vim: fdm=marker

