set(UNIX_CFGCMD ${DEPS_BUILD_DIR}/src/curl/configure
  --without-librtmp --without-libpsl --without-libidn2
  --disable-ldap --disable-ldaps --with-ssl=${DEPS_INSTALL_DIR}
  --prefix=${DEPS_INSTALL_DIR} --libdir=${DEPS_INSTALL_DIR}/lib
  --exec-prefix=${DEPS_INSTALL_DIR}
  CC=${DEPS_C_COMPILER} CPPFLAGS=-I${DEPS_INSTALL_DIR}/include
  LDFLAGS=-L${DEPS_INSTALL_DIR}/lib
  PKG_CONFIG_PATH=${DEPS_INSTALL_DIR}/lib/pkgconfig
  MAKE=${MAKE_PRG})

ExternalProject_Add(curl
  PREFIX ${DEPS_BUILD_DIR}
  URL ${CURL_URL}
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/curl
  DOWNLOAD_COMMAND ${CMAKE_COMMAND}
    -DPREFIX=${DEPS_BUILD_DIR}
    -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/curl
    -DURL=${CURL_URL}
    -DEXPECTED_SHA256=${CURL_SHA256}
    -DTARGET=curl
    -DUSE_EXISTING_SRC_DIR=OFF
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadAndExtractFile.cmake
  CONFIGURE_COMMAND "${UNIX_CFGCMD}"
  BUILD_COMMAND "${MAKE_PRG}"
  INSTALL_COMMAND "${MAKE_PRG}" install
)

add_dependencies(curl openssl)
list(APPEND ALL_DEPS curl)
