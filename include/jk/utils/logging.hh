// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <spdlog/async.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

namespace jk::utils {

//! Get a logger, if loger is not exists, then create a new one
std::shared_ptr<spdlog::logger> Logger(std::string_view);

}  // namespace jk::utils

// vim: fdm=marker

