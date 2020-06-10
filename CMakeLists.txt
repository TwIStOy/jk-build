cmake_minimum_required(VERSION 3.12)
project(jk LANGUAGES CXX)

set(DEFAULT_CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PYBIND11_CPP_STANDARD -std=c++1z)

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

include_directories(./include)

# --- third-party section ---
add_subdirectory(third-party/fmt)
target_link_libraries(libjk PUBLIC fmt::fmt)

add_subdirectory(third-party/pybind11)
TARGET_LINK_LIBRARIES(libjk PUBLIC pybind11::embed)
TARGET_LINK_LIBRARIES(libjk PUBLIC pybind11::pybind11)
TARGET_LINK_LIBRARIES(libjk PUBLIC pybind11::module)

target_link_libraries(jk PRIVATE libjk)
target_link_libraries(jk_test PRIVATE libjk)

set(JK_SOURCE_FILES src/jk/tmp.cc src/jk/core/rules/build_rule.cc)
set(JK_TEST_FILES test/test.cc)

target_sources(libjk PRIVATE ${JK_SOURCE_FILES})
target_sources(jk PRIVATE src/main.cc)
target_sources(jk_test PRIVATE ${JK_TEST_FILES})


