// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

namespace jk {

#define JK_VERSION_MAJOR 0
#define JK_VERSION_MINOR 1
#define JK_VERSION_PATCH 0

#define _MK_VER(x, y, z) #x "." #y "." #z

#define JK_VERSION _MK_VER(JK_VERSION_MAJOR, JK_VERSION_MINOR, JK_VERSION_PATCH)

}  // namespace jk
