// Copyright (c) 2020 Hawtian Wang
//

#include <stdexcept>
#include <string_view>

#include "fmt/format.h"

namespace jk {
namespace core {

class JKBuildError : std::runtime_error {
 public:
  template <typename... Args>
  JKBuildError(std::string_view fmt, const Args&... args)
      : std::runtime_error(fmt::format(fmt, args...)) {}
};

}  // namespace core
}  // namespace jk

