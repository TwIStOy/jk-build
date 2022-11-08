cmake_minimum_required(VERSION 3.12)
project(jk LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(DEFAULT_CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(JK_FULL_STATIC "Make all static for jk" ON)

if (JK_FULL_STATIC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -Os")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse3 -fPIC -g2 -Wno-attributes")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -msse3 -fPIC -g2 -Wno-attributes")
set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -g2")
set(CMAKE_LINKER_FLAGS_DEBUG
    "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -g2")
# set(CMAKE_CXX_FLAGS_DEBUG
#     "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -g2 -fsanitize=address")
# set(CMAKE_LINKER_FLAGS_DEBUG
#     "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -g2 -fsanitize=address")

if(CMAKE_BUILD_TYPE EQUAL "Debug")
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

execute_process(COMMAND sh third-party/build.sh
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(jk "")
add_library(libjk STATIC "")
add_executable(jk_test "")

set_property(TARGET libjk PROPERTY CXX_STANDARD 20)
set_property(TARGET libjk PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET libjk PROPERTY CXX_EXTENSIONS OFF)

set_property(TARGET jk PROPERTY CXX_STANDARD 20)
set_property(TARGET jk PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET jk PROPERTY CXX_EXTENSIONS OFF)

set_property(TARGET jk_test PROPERTY CXX_STANDARD 20)
set_property(TARGET jk_test PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET jk_test PROPERTY CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
  target_link_libraries(libjk PUBLIC stdc++fs)
endif()

if (JK_FULL_STATIC)
  target_link_options(libjk PUBLIC -static-libgcc -static-libstdc++)
else()
  target_link_options(libjk PUBLIC -static-libstdc++)
endif()

# third-party prepare
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
set(DEPS_PREFIX
    "${PROJECT_SOURCE_DIR}/third-party/.deps/usr"
    CACHE PATH "Path prefix for finding dependencies")
list(INSERT CMAKE_PREFIX_PATH 0 ${DEPS_PREFIX})
set(ENV{PKG_CONFIG_PATH} "${DEPS_PREFIX}/lib/pkgconfig")
message(STATUS "Deps prefix: ${DEPS_PREFIX}")
message(STATUS "PkgConfig Path: $ENV{PKG_CONFIG_PATH}")

find_package(PkgConfig REQUIRED)

# bundled python3
pkg_search_module(PKG_python2 REQUIRED IMPORTED_TARGET python2)
set(PKG_python2_STATIC_LIBRARIES "${DEPS_PREFIX}/lib/libpython2.7.a")
add_library(BundledPython2 INTERFACE)
set_property(
  TARGET BundledPython2
  APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
           $<BUILD_INTERFACE:${PKG_python2_INCLUDE_DIRS}>)
set_property(
  TARGET BundledPython2
  APPEND
  PROPERTY INTERFACE_COMPILE_OPTIONS
           $<BUILD_INTERFACE:${PKG_python2_STATIC_CFLAGS}>)
set_property(
  TARGET BundledPython2
  APPEND
  PROPERTY INTERFACE_LINK_LIBRARIES
           $<BUILD_INTERFACE:${PKG_python2_STATIC_LIBRARIES}>
           ${PKG_python2_STATIC_LDFLAGS})
target_link_libraries(libjk PUBLIC BundledPython2)

# bundled openssl
pkg_search_module(PKG_openssl REQUIRED IMPORTED_TARGET openssl)
add_library(BundledOpenssl INTERFACE)
add_library(BundledOpenssl::ssl IMPORTED STATIC)
add_library(BundledOpenssl::crypto IMPORTED STATIC)
set_target_properties(BundledOpenssl::ssl
                      PROPERTIES IMPORTED_LOCATION ${DEPS_PREFIX}/lib/libssl.a)
set_target_properties(
  BundledOpenssl::crypto PROPERTIES IMPORTED_LOCATION
                                    ${DEPS_PREFIX}/lib/libcrypto.a)
set_property(
  TARGET BundledOpenssl
  APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
           $<BUILD_INTERFACE:${PKG_openssl_INCLUDE_DIRS}>)
set_property(
  TARGET BundledOpenssl
  APPEND
  PROPERTY INTERFACE_COMPILE_OPTIONS
           $<BUILD_INTERFACE:${PKG_openssl_STATIC_CFLAGS}>)
target_link_libraries(BundledOpenssl::ssl INTERFACE BundledOpenssl::crypto)
target_link_libraries(BundledOpenssl INTERFACE BundledOpenssl::ssl)
target_link_libraries(libjk PUBLIC BundledOpenssl)

# bundled libcurl
pkg_search_module(PKG_curl REQUIRED IMPORTED_TARGET libcurl)
add_library(BundledCurl INTERFACE)
add_library(BundledCurl::curl IMPORTED STATIC)
set_target_properties(BundledCurl::curl PROPERTIES IMPORTED_LOCATION
                                                   ${DEPS_PREFIX}/lib/libcurl.a)
set_property(
  TARGET BundledCurl
  APPEND
  PROPERTY INTERFACE_INCLUDE_DIRECTORIES
           $<BUILD_INTERFACE:${PKG_curl_INCLUDE_DIRS}>)
set_property(
  TARGET BundledCurl
  APPEND
  PROPERTY INTERFACE_COMPILE_OPTIONS
           $<BUILD_INTERFACE:${PKG_curl_STATIC_CFLAGS}>)
set_property(
  TARGET BundledCurl::curl
  APPEND
  PROPERTY INTERFACE_LINK_LIBRARIES -lz)
target_link_libraries(BundledCurl::curl INTERFACE BundledOpenssl::ssl)
target_link_libraries(BundledCurl INTERFACE BundledCurl::curl)
target_link_libraries(BundledCurl INTERFACE BundledOpenssl)
target_link_libraries(libjk PUBLIC BundledCurl)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(JSON_BuildTests
    OFF
    CACHE INTERNAL "")

find_package(Threads REQUIRED)

# --- boost section ---
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(
  Boost
  COMPONENTS system filesystem
  REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(libjk PUBLIC ${Boost_FILESYSTEM_LIBRARIES})
  target_link_libraries(libjk PUBLIC ${Boost_SYSTEM_LIBRARIES})
endif()

include_directories(./source)
include_directories(third-party/include)
include_directories(third-party/toml11)

# --- third-party section ---
add_subdirectory(third-party/fmt)
target_link_libraries(libjk PUBLIC fmt::fmt)

set(PYBIND11_NOPYTHON ON)
add_subdirectory(third-party/pybind11)
target_link_libraries(libjk PUBLIC pybind11::embed)
target_link_libraries(libjk PUBLIC pybind11::pybind11)
target_link_libraries(libjk PUBLIC pybind11::module)

add_subdirectory(third-party/spdlog)
set(SPDLOG_FMT_EXTERNAL ON)
target_link_libraries(libjk PUBLIC spdlog::spdlog)

target_link_libraries(jk PRIVATE libjk)
target_link_libraries(jk_test PRIVATE libjk)

add_subdirectory(third-party/nlohmann_json)
target_link_libraries(libjk PUBLIC nlohmann_json::nlohmann_json)

add_subdirectory(third-party/abseil-cpp)
target_link_libraries(
  libjk
  PUBLIC absl_algorithm
         absl_base
         absl_cleanup
         absl_debugging
         absl_flags
         absl_hash
         absl_memory
         absl_meta
         absl_numeric
         absl_status
         absl_strings
         absl_synchronization
         absl_time
         absl_utility)

add_subdirectory(third-party/range-v3)
target_link_libraries(libjk PUBLIC range-v3)

# --- source fies ---

file(GLOB_RECURSE JK_SOURCE_FILES ./source/jk/*.cc)
file(GLOB_RECURSE JK_TEST_FILES ./source/test/*.cc)

target_sources(libjk PRIVATE ${JK_SOURCE_FILES})
target_sources(jk PRIVATE source/main.cc)
target_sources(jk_test PRIVATE ${JK_TEST_FILES})

install(TARGETS jk)

enable_testing()

add_test(AllTest jk_test)

# -- code coverage section ---
if(CMAKE_BUILD_TYPE EQUAL "Debug")
  setup_target_for_coverage_gcovr_html(
    NAME
    jk_coverage
    EXECUTABLE
    jk_test
    BASE_DIRECTORY
    "${PROJECT_SOURCE_DIR}/src"
    EXCLUDE
    "third-party/*")
endif()
