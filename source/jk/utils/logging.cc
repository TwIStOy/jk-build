// Copyright (c) 2020 Hawtian Wang
//

#include "jk/utils/logging.hh"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

#include "spdlog/sinks/stdout_color_sinks.h"

namespace jk::utils {

static std::vector<spdlog::sink_ptr> Sinks() {
  auto fileSink =
      std::make_shared<spdlog::sinks::basic_file_sink_mt>("jk-build.log");

  fileSink->set_level(spdlog::level::debug);
  // [I2020-06-25 12:01:02.0003] 123-123 <logger> logging.cc:23] my logs
  fileSink->set_pattern("[%L%Y-%m-%d %H:%M:%S.%e] %P-%t <%n> %@] %v");

  auto consoleSink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
  consoleSink->set_pattern("%+");
  consoleSink->set_level(spdlog::level::info);

  return {fileSink, consoleSink};
}

std::shared_ptr<spdlog::logger> Logger(std::string_view name) {
  static bool initialized = false;
  static auto sinks = Sinks();

  if (!initialized) {
    spdlog::init_thread_pool(16 * 1024, 1);
  }

  auto logger = spdlog::get(name.cbegin());
  if (logger) {
    return logger;
  }

  // logger = std::make_shared<spdlog::async_logger>(
  //     name.cbegin(), sinks.begin(), sinks.end(), spdlog::thread_pool(),
  //     spdlog::async_overflow_policy::overrun_oldest);
  logger = std::make_shared<spdlog::logger>(name.cbegin(), sinks.begin(),
                                            sinks.end());
  spdlog::register_logger(logger);
  return logger;
}

}  // namespace jk::utils

// vim: fdm=marker

