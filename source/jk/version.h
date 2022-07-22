// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

namespace jk {

#define JK_VERSION_MAJOR 1
#define JK_VERSION_MINOR 3
#define JK_VERSION_PATCH 0

#define _MK_STR(x)       #x
#define _MK_VER(x, y, z) _MK_STR(x) "." _MK_STR(y) "." _MK_STR(z)

#define JK_VERSION _MK_VER(JK_VERSION_MAJOR, JK_VERSION_MINOR, JK_VERSION_PATCH)

}  // namespace jk
