// Copyright (c) 2020 Hawtian Wang
//

#pragma once  // NOLINT(build/header_guard)

#include <string>
namespace jk::core::cache {

struct CacheWriter {
  virtual ~CacheWriter() = default;

  virtual void Flush() = 0;

  /*
   * virtual void WriterKV(const std::string &k, const std::string &v) = 0;
   *
   * virtual void WriterVec(const std::string) = 0;
   */
};

}  // namespace jk::core::cache

// vim: fdm=marker
