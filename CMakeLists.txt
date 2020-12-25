cmake_minimum_required(VERSION 3.12)
project(jk LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(DEFAULT_CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

if (CMAKE_BUILD_TYPE EQUAL "Debug")
  include(CodeCoverage)
  append_coverage_compiler_flags()
endif()

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


# find_package(PkgConfig REQUIRED)

set(Python3_USE_STATIC_LIBS ON)
find_package(Python3 3.9 COMPONENTS Interpreter Development)
if (Python3_FOUND)
  message(STATUS "Found Python3 executable: ${Python3_EXECUTABLE}")
  target_link_libraries(libjk PUBLIC Python3::Python)
  target_link_libraries(libjk PUBLIC Python3::Module)
  target_link_libraries(libjk PUBLIC "-lz")
else()
  message(FATAL_ERROR "Python3 is required.")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(JSON_BuildTests OFF CACHE INTERNAL "")

find_package(Threads REQUIRED)

set(CURL_USE_STATIC_LIBS ON)
find_package(CURL REQUIRED)
if (CURL_FOUND)
  include_directories(${CURL_INCLUDE_DIRS})
  target_link_libraries(libjk PUBLIC ${CURL_LIBRARIES})
endif()

set(OpenSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

target_link_libraries(libjk PUBLIC OpenSSL::SSL)
target_link_libraries(libjk PUBLIC OpenSSL::Crypto)

# --- boost section ---
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS system filesystem REQUIRED)
if (Boost_FOUND)
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
if (CMAKE_BUILD_TYPE EQUAL "Debug")
setup_target_for_coverage_gcovr_html(
  NAME jk_coverage
  EXECUTABLE jk_test
  BASE_DIRECTORY "${PROJECT_SOURCE_DIR}/src"
  EXCLUDE "third-party/*"
)
endif()

