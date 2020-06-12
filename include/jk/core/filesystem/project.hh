// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace jk {
namespace core {
namespace filesystem {

fs::path ProjectRoot();

}  // namespace filesystem
}  // namespace core
}  // namespace jk

