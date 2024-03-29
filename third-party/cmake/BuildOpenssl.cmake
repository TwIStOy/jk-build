if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  if (${CMAKE_APPLE_SILICON_PROCESSOR} MATCHES "arm64")
    set(OPENSSL_OS darwin64-arm64-cc)
  else()
    set(OPENSSL_OS darwin64-x86_64-cc)
  endif()
else()
  set(OPENSSL_OS linux-x86_64)
endif()


set(UNIX_CFGCMD ${DEPS_BUILD_DIR}/src/openssl/Configure --release
  --prefix=${DEPS_INSTALL_DIR} --openssldir=/usr/lib/ssl
  --libdir=${DEPS_INSTALL_DIR}/lib no-shared no-tests ${OPENSSL_OS}
  CC=${DEPS_C_COMPILER} CPPFLAGS=-fPIC MAKE=${MAKE_PRG})

ExternalProject_Add(openssl
  PREFIX ${DEPS_BUILD_DIR}
  URL ${OPENSSL_URL}
  DOWNLOAD_DIR ${DEPS_DOWNLOAD_DIR}/openssl
  DOWNLOAD_COMMAND ${CMAKE_COMMAND}
    -DPREFIX=${DEPS_BUILD_DIR}
    -DDOWNLOAD_DIR=${DEPS_DOWNLOAD_DIR}/openssl
    -DURL=${OPENSSL_URL}
    -DEXPECTED_SHA256=${OPENSSL_SHA256}
    -DTARGET=openssl
    -DUSE_EXISTING_SRC_DIR=OFF
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadAndExtractFile.cmake
  CONFIGURE_COMMAND "${UNIX_CFGCMD}"
  BUILD_COMMAND "${MAKE_PRG}"
  INSTALL_COMMAND "${MAKE_PRG}" install_sw
)

list(APPEND ALL_DEPS openssl)
