THIS_DIR = $(shell pwd)

all: jk

CMAKE_PRG ?= $(shell (command -v cmake3 || echo cmake))
CMAKE_BUILD_TYPE ?= Debug
CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)
# Extra CMake flags which extend the default set
CMAKE_EXTRA_FLAGS ?=

BUILD_TYPE ?= $(shell (type ninja > /dev/null 2>&1 && echo "Ninja") || \
							echo "Unix Makefiles")
DEPS_BUILD_DIR ?= .deps

ifeq (,$(BUILD_TOOL))
  ifeq (Ninja,$(BUILD_TYPE))
    ifneq ($(shell $(CMAKE_PRG) --help 2>/dev/null | grep Ninja),)
      BUILD_TOOL := ninja
    else
      # User's version of CMake doesn't support Ninja
      BUILD_TOOL = $(MAKE)
      BUILD_TYPE := Unix Makefiles
    endif
  else
    BUILD_TOOL = $(MAKE)
  endif
endif

BUILD_CMD = $(BUILD_TOOL)

DEPS_CMAKE_FLAGS ?=
USE_BUNDLED ?=

ifneq (,$(USE_BUNDLED))
	BUNDLED_CMAKE_FLAGS := -DUSE_BUNDLED=$(USE_BUNDLED)
endif

SINGLE_MAKE = export MAKEFLAGS= ; $(MAKE)

jk: build/.ran-cmake deps
	+$(BUILD_CMD) -C build

libjk: build/.ran-cmake deps
	+$(BUILD_CMD) -C build libjk

cmake:
	touch CMakeLists.txt
	$(MAKE) build/.ran-cmake

build/.ran-cmake: | deps
	cd build && $(CMAKE_PRG) -G '$(BUILD_TYPE)' $(CMAKE_FLAGS) $(CMAKE_EXTRA_FLAGS) $(THIS_DIR)
	touch $@

deps: | build/.rand-third-party-cmake


