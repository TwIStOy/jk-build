set(UNIX_CFGCMD sh ${DEPS_BUILD_DIR}/src/python3/configure --enable-optimizations
  --prefix=${DEPS_INSTALL_DIR} --libdir=${DEPS_INSTALL_DIR}/lib
  CC=${DEPS_C_COMPILER} CPPFLAGS=-fPIC MAKE=${MAKE_PRG})

ExternalProject_Add(python3
  PREFIX ${DEPS_BUILD_DIR}
  URL ${PYTHON_URL}
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/python3
  DOWNLOAD_COMMAND ${CMAKE_COMMAND}
    -DPREFIX=${DEPS_BUILD_DIR}
    -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/python3
    -DURL=${PYTHON_URL}
    -DEXPECTED_SHA256=${PYTHON_SHA256}
    -DTARGET=python3
    -DUSE_EXISTING_SRC_DIR=OFF
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadAndExtractFile.cmake
  CONFIGURE_COMMAND "${UNIX_CFGCMD}"
  BUILD_COMMAND "${MAKE_PRG}"
  INSTALL_COMMAND "${MAKE_PRG}" install
)

list(APPEND ALL_DEPS python3)