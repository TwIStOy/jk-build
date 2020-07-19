cmake_minimum_required(VERSION 3.12)
project(jk LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(DEFAULT_CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PYBIND11_CPP_STANDARD -std=c++1z)

set(CMAKE_CXX_CPPLINT "cpplint")

# include(CodeCoverage)
# append_coverage_compiler_flags()

# set(CMAKE_CXX_FLAGS_DEBUG -fsanitize=address
#                           -fno-inline
#                           -fno-omit-frame-pointer
#                           -fno-builtin
#                           -fno-optimize-sibling-calls
#                           -Wframe-larger-than=65535
#                           -fno-omit-frame-pointer
# )


add_executable(jk "")
add_library(libjk STATIC "")
add_executable(jk_test "")

set_property(TARGET libjk PROPERTY CXX_STANDARD 17)
set_property(TARGET libjk PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET libjk PROPERTY CXX_EXTENSIONS OFF)

set_property(TARGET jk PROPERTY CXX_STANDARD 17)
set_property(TARGET jk PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET jk PROPERTY CXX_EXTENSIONS OFF)

set_property(TARGET jk_test PROPERTY CXX_STANDARD 17)
set_property(TARGET jk_test PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET jk_test PROPERTY CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
  target_link_libraries(libjk PUBLIC stdc++fs)
endif()

find_package(PkgConfig REQUIRED)
PKG_CHECK_MODULES(PYTHON3 REQUIRED python3)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

find_package(Threads REQUIRED)

find_package(OpenSSL REQUIRED)

# --- boost section ---
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED)
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(libjk PUBLIC ${Boost_LIBRARIES})
endif()

include_directories(.)
include_directories(./include)
include_directories(third-party/include)

# --- third-party section ---
add_subdirectory(third-party/fmt)
target_link_libraries(libjk PUBLIC fmt::fmt)

add_subdirectory(third-party/pybind11)
target_link_libraries(libjk PUBLIC pybind11::embed)
target_link_libraries(libjk PUBLIC pybind11::pybind11)
target_link_libraries(libjk PUBLIC pybind11::module)

add_subdirectory(third-party/spdlog)
set(SPDLOG_FMT_EXTERNAL ON)
target_link_libraries(libjk PUBLIC spdlog::spdlog)

target_link_libraries(jk PRIVATE libjk)
target_link_libraries(jk_test PRIVATE libjk)


# --- source fies ---

file(GLOB_RECURSE JK_SOURCE_FILES ./src/*.cc)
file(GLOB_RECURSE JK_TEST_FILES ./test/*.cc)

target_sources(libjk PRIVATE ${JK_SOURCE_FILES})
target_sources(jk PRIVATE src/main.cc)
target_sources(jk_test PRIVATE ${JK_TEST_FILES})

enable_testing()

add_test(AllTest jk_test)

# -- code coverage section ---
# setup_target_for_coverage_gcovr_html(
#   NAME jk_coverage
#   EXECUTABLE jk_test
#   BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
#   EXCLUDE "third-party/*"
# )

