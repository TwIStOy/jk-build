// Copyright (c) 2020 Hawtian Wang
//

#include "jk/core/filesystem/configuration.hh"

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "jk/utils/logging.hh"

namespace jk::core::filesystem {

static auto logger = utils::Logger("Configuration");

#define EXTRACT_VALUE(name, default_value)                            \
  name(extract_value<std::remove_cv_t<std::decay_t<decltype(name)>>>( \
      value, #name, default_value))

template<typename T>
static T extract_value(const toml::value &value, const char *name,
                       const T &default_value) {
  if (const auto exp = toml::expect<T>(value.at(name)); exp.is_ok()) {
    logger->info("{} use value from project-level configuration", name);
    return exp.unwrap();
  } else {
    logger->info("{} use its default value", name);
    return default_value;
  }
}

// {{{
#define DEFAULT_COMPILE_FLAGS                                           \
  {                                                                     \
    "-MMD", "-msse3", "-fPIC", "-fstrict-aliasing", "-Wall", "-Wextra", \
        "-Wtrigraphs", "-Wuninitialized", "-Wwrite-strings",            \
        "-Wpointer-arith", "-Wredundant-decls", "-Wunused",             \
        "-Wmissing-include-dirs", "-Wno-missing-field-initializers",    \
        "-Werror", "-w",                                                \
  }

#define DEFAULT_C_FLAGS \
  { "-D_GNU_SOURCE", "-Werror-implicit-function-declaration", }

#define DEFAULT_CPP_FLAGS                                    \
  {                                                          \
    "-Wvla", "-Wnon-virtual-dtor", "-Woverloaded-virtual",   \
        "-Wno-invalid-offsetof", "-Werror=non-virtual-dtor", \
        "-D__STDC_FORMAT_MACROS", "-DUSE_SYMBOLIZE",         \
  }

#define DEFAULT_DEBUG_CFLAGS_EXTRA                                            \
  {                                                                           \
    "-O0", "-ggdb3", "-Wformat=2", "-Wstrict-aliasing", "-fsanitize=address", \
        "-fno-inline", "-fno-omit-frame-pointer", "-fno-builtin",             \
        "-fno-optimize-sibling-calls", "-Wframe-larger-than=65535",           \
        "-fno-omit-frame-pointer",                                            \
  }

#define DEFAULT_DEBUG_CPPFLAGS_EXTRA                                          \
  {                                                                           \
    "-O0", "-ggdb3", "-Wformat=2", "-Wstrict-aliasing", "-fsanitize=address", \
        "-fno-inline", "-fno-omit-frame-pointer", "-fno-builtin",             \
        "-fno-optimize-sibling-calls", "-Wframe-larger-than=65535",           \
        "-fno-omit-frame-pointer", "-ftest-coverage", "-fprofile-arcs"        \
  }

#define DEFAULT_RELEASE_CFLAGS_EXTRA                                          \
  {                                                                           \
    "-DNDEBUG", "-O3", "-ggdb3", "-Wformat=2", "-Wstrict-aliasing",           \
        "-fno-builtin-malloc", "-fno-builtin-calloc", "-fno-builtin-realloc", \
        "-fno-builtin-free3", "-Wframe-larger-than=65535",                    \
        "-fno-omit-frame-pointer",                                            \
  }

#define DEFAULT_RELEASE_CPPFLAGS_EXTRA                                         \
  {                                                                            \
    "-DNDEBUG", "-DUSE_TCMALLOC=1", "-DNDEBUG", "-O3", "-ggdb3", "-Wformat=2", \
        "-Wstrict-aliasing", "-fno-builtin-malloc", "-fno-builtin-calloc",     \
        "-fno-builtin-realloc", "-fno-builtin-free",                           \
        "-Wframe-larger-than=65535", "-fno-omit-frame-pointer",                \
  }

#define DEFAULT_PROFILING_CFLAGS_EXTRA                                        \
  {                                                                           \
    "-DNDEBUG", "-O3", "-ggdb3", "-Wformat=2", "-Wstrict-aliasing",           \
        "-fno-builtin-malloc", "-fno-builtin-calloc", "-fno-builtin-realloc", \
        "-fno-builtin-free3", "-Wframe-larger-than=65535",                    \
        "-fno-omit-frame-pointer",                                            \
  }

#define DEFAULT_PROFILING_CPPFLAGS_EXTRA                                    \
  {                                                                         \
    "-DNDEBUG", "-DUSE_TCMALLOC=1", "-DHEAP_PROFILING", "-DNDEBUG", "-O3",  \
        "-ggdb3", "-Wformat=2", "-Wstrict-aliasing", "-fno-builtin-malloc", \
        "-fno-builtin-calloc", "-fno-builtin-realloc", "-fno-builtin-free", \
        "-Wframe-larger-than=65535", "-fno-omit-frame-pointer",             \
  }

#define DEFAULT_LDFLAGS                                                    \
  {                                                                        \
    "-L.build/.lib/m${PLATFORM}/lib", "-m${PLATFORM}", "-levent",          \
        "-levent_pthreads", "-pthread", "-lpthread", "-Wl,--no-as-needed", \
        "-ldl", "-lrt",                                                    \
  }

#define DEFAULT_RELEASE_LDFLAGS_EXTRA \
  { "-Wl,-Bstatic,-ltcmalloc_minimal", "-Wl,-Bdynamic", }

#define DEFAULT_PROFILING_LDFLAGS_EXTRA                            \
  {                                                                \
    "-Wl,--whole-archive", "-Wl,-Bstatic,-ltcmalloc_and_profiler", \
        "-Wl,--no-whole-archive", "-Wl,-Bdynamic",                 \
  }

#define DEFAULT_DEBUG_LDFLAGS_BEFORE \
  { "-ftest-coverage", "-fprofile-arcs", }

#define DEFAULT_DEBUG_LDFLAGS_EXTRA \
  { "-static-libasan", "-Wl,-Bstatic,-lasan", "-Wl,-Bdynamic", "-ldl", }
// }}}

Configuration::Configuration(const toml::value &value)
    : EXTRACT_VALUE(cpplint_path, "cpplint"),
      EXTRACT_VALUE(cxx_standard, "11"),
      EXTRACT_VALUE(compile_flags, DEFAULT_COMPILE_FLAGS),
      EXTRACT_VALUE(cflags, DEFAULT_C_FLAGS),
      EXTRACT_VALUE(cppflags, DEFAULT_CPP_FLAGS),
      EXTRACT_VALUE(debug_cflags_extra, DEFAULT_DEBUG_CFLAGS_EXTRA),
      EXTRACT_VALUE(debug_cppflags_extra, DEFAULT_DEBUG_CPPFLAGS_EXTRA),
      EXTRACT_VALUE(release_cflags_extra, DEFAULT_RELEASE_CFLAGS_EXTRA),
      EXTRACT_VALUE(release_cppflags_extra, DEFAULT_RELEASE_CPPFLAGS_EXTRA),
      EXTRACT_VALUE(profiling_cflags_extra, DEFAULT_PROFILING_CFLAGS_EXTRA),
      EXTRACT_VALUE(profiling_cppflags_extra, DEFAULT_PROFILING_CPPFLAGS_EXTRA),
      EXTRACT_VALUE(ld_flags, DEFAULT_LDFLAGS),
      EXTRACT_VALUE(release_ld_flags_extra, DEFAULT_RELEASE_LDFLAGS_EXTRA),
      EXTRACT_VALUE(profiling_ld_flags_extra, DEFAULT_PROFILING_LDFLAGS_EXTRA),
      EXTRACT_VALUE(debug_ld_flags_extra, DEFAULT_DEBUG_LDFLAGS_EXTRA) {
}

}  // namespace jk::core::filesystem

// vim: fdm=marker

