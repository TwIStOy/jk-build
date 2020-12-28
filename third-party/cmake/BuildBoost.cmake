set(UNIX_CFGCMD sh ${DEPS_BUILD_DIR}/src/boost/bootstrap.sh
  --prefix=${DEPS_INSTALL_DIR} --exec-prefix=${DEPS_INSTALL_DIR})

set(UNIX_INSTALL_CMD ${DEPS_BUILD_DIR}/src/boost/b2
               cxxflags=-O3 --without-python
               link=static runtime-link=static variant=release install)

ExternalProject_Add(boost
  BUILD_IN_SOURCE ON
  PREFIX ${DEPS_BUILD_DIR}
  URL ${BOOST_URL}
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/boost
  DOWNLOAD_COMMAND ${CMAKE_COMMAND}
    -DPREFIX=${DEPS_BUILD_DIR}
    -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/boost
    -DURL=${BOOST_URL}
    -DEXPECTED_SHA256=${BOOST_SHA256}
    -DTARGET=boost
    -DUSE_EXISTING_SRC_DIR=OFF
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadAndExtractFile.cmake
  CONFIGURE_COMMAND "${UNIX_CFGCMD}"
  BUILD_COMMAND "${UNIX_INSTALL_CMD}"
  INSTALL_COMMAND "${UNIX_INSTALL_CMD}"
)

list(APPEND ALL_DEPS boost)
