cmake_minimum_required(VERSION 3.12)
project(JK_DEPS C)
enable_language(CXX)

include(CheckCCompilerFlag)
include(ExternalProject)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${PROJECT_SOURCE_DIR}/../cmake")

# Default 'Release'
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(DEFAULT_MAKE_CFLAGS CFLAGS+=-g)

check_c_compiler_flag(-Og HAS_OG_FLAG)
if(HAS_OG_FLAG)
  set(DEFAULT_MAKE_CFLAGS CFLAGS+=-Og ${DEFAULT_MAKE_CFLAGS})
endif()

set(DEPS_INSTALL_DIR "${PROJECT_SOURCE_DIR}/.deps/usr" CACHE PATH "Dependencies install directory.")
message("Deps installed at ${DEPS_INSTALL_DIR}")
set(DEPS_BIN_DIR "${DEPS_INSTALL_DIR}/bin" CACHE PATH "Dependencies binary install directory.")
set(DEPS_LIB_DIR "${DEPS_INSTALL_DIR}/lib" CACHE PATH "Dependencies library install directory.")
set(DEPS_BUILD_DIR "${CMAKE_BINARY_DIR}/build" CACHE PATH "Dependencies build directory.")
set(DEPS_DOWNLOAD_DIR "${DEPS_BUILD_DIR}/downloads" CACHE PATH "Dependencies download directory.")

if(UNIX)
  find_program(MAKE_PRG NAMES gmake make)
  if(MAKE_PRG)
    execute_process(
      COMMAND "${MAKE_PRG}" --version
      OUTPUT_VARIABLE MAKE_VERSION_INFO)
    if(NOT "${OUTPUT_VARIABLE}" MATCHES ".*GNU.*")
      unset(MAKE_PRG)
    endif()
  endif()
  if(NOT MAKE_PRG)
    message(FATAL_ERROR "GNU Make is required to build the dependencies.")
  else()
    message(STATUS "Found GNU Make at ${MAKE_PRG}")
  endif()
endif()

# When using make, use the $(MAKE) variable to avoid warning about the job
# server.
if(CMAKE_GENERATOR MATCHES "Makefiles")
  set(MAKE_PRG "$(MAKE)")
endif()

if(MINGW AND CMAKE_GENERATOR MATCHES "Ninja")
  find_program(MAKE_PRG NAMES mingw32-make)
  if(NOT MAKE_PRG)
    message(FATAL_ERROR "GNU Make for mingw32 is required to build the dependecies.")
  else()
    message(STATUS "Found GNU Make for mingw32: ${MAKE_PRG}")
  endif()
endif()

set(PYTHON_URL https://github.com/python/cpython/archive/v3.9.1.tar.gz)
set(PYTHON_SHA256 4de9ef9631daa85971420d1f5912296ec58105d0a95057720a7c008217e9e827)
include(BuildPython)

set(OPENSSL_URL https://www.openssl.org/source/openssl-1.1.1i.tar.gz)
set(OPENSSL_SHA256 e8be6a35fe41d10603c3cc635e93289ed00bf34b79671a3a4de64fcee00d5242)
include(BuildOpenssl)

set(CURL_URL https://curl.se/download/curl-7.74.0.tar.gz)
set(CURL_SHA256 e56b3921eeb7a2951959c02db0912b5fcd5fdba5aca071da819e1accf338bbd7)
include(BuildCurl)

set(BOOST_URL https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.gz)
set(BOOST_SHA256 aeb26f80e80945e82ee93e5939baebdca47b9dee80a07d3144be1e1a6a66dd6a)
include(BuildBoost)

add_custom_target(third-party ALL
  COMMAND ${CMAKE_COMMAND} -E touch .third-party
  DEPENDS ${ALL_DEPS}
)

