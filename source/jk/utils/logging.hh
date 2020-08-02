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

#define LOG(logger, lvl, ...) \
  SPDLOG_LOGGER_CALL(logger, ::spdlog::level::lvl, __VA_ARGS__)

#define JK_THROW(e)                                                        \
  do {                                                                     \
    LOG(::jk::utils::Logger("default"), critical, "JKBuildError: {}", #e); \
    throw((e));                                                            \
  } while (0);

}  // namespace jk::utils

// vim: fdm=marker

