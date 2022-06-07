# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Going to be compatible with Qt 6.2 LTS.
# See https://doc.qt.io/qt-6.2/cmake-get-started.html
cmake_minimum_required(VERSION 3.16)

project("Bitcoin Core"
  VERSION 23.0.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX C ASM
)

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(CLIENT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CLIENT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CLIENT_VERSION_BUILD ${PROJECT_VERSION_PATCH})
set(CLIENT_VERSION_IS_RELEASE "false")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")
set(PACKAGE_URL ${PROJECT_HOMEPAGE_URL})
set(COPYRIGHT_YEAR "2022")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_SUBSTITUTION ${PROJECT_NAME})
set(COPYRIGHT_HOLDERS_FINAL "The Bitcoin Core developers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-pipe -O2)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  link_libraries(-Wl,-O2)
endif()

if(WIN32)
  add_compile_definitions(_MT WIN32 _WINDOWS _WIN32_WINNT=0x0601 _WIN32_IE=0x0501 WIN32_LEAN_AND_MEAN)
  link_libraries(-Wl,--major-subsystem-version -Wl,6 -Wl,--minor-subsystem-version -Wl,1)
endif()

if(CMAKE_CROSSCOMPILING)
  add_definitions(${CPPFLAGS_FROM_DEPENDS})
  add_compile_options(${CFLAGS_FROM_DEPENDS} ${CXXFLAGS_FROM_DEPENDS})
  link_libraries(${LDFLAGS_FROM_DEPENDS})
endif()

include(cmake/introspection.cmake)

configure_file(cmake/bitcoin-config.h.in src/config/bitcoin-config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/src)
add_compile_definitions(HAVE_CONFIG_H)

option(USE_CCACHE "Use ccache for building" ON)
if(USE_CCACHE)
  find_program(CCACHE ccache)
  if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
  endif()
endif()

option(BUILD_DAEMON "Build bitcoind" ON)
option(BUILD_CLI "Build bitcoin-cli" ON)

include(cmake/subtree-crc32c.cmake)
include(cmake/subtree-leveldb.cmake)
include(cmake/subtree-minisketch.cmake)
include(cmake/subtree-secp256k1.cmake)
include(cmake/subtree-univalue.cmake)

include(FindPkgConfig)

if(BUILD_DAEMON OR BUILD_CLI)
  # Find Boost headers only.
  find_package(Boost 1.64.0 REQUIRED)

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  pkg_check_modules(libevent REQUIRED libevent>=2.1.8 IMPORTED_TARGET)
  if(WIN32)
    target_link_libraries(PkgConfig::libevent INTERFACE iphlpapi ws2_32)
  endif()
endif()

if(NOT WIN32 AND BUILD_DAEMON)
  pkg_check_modules(libevent_pthreads REQUIRED libevent_pthreads>=2.1.8 IMPORTED_TARGET)
endif()

add_subdirectory(src)

message(NOTICE "\n")
if(CMAKE_CROSSCOMPILING)
  message(NOTICE "Cross compiling for ${CMAKE_SYSTEM_NAME}")
endif()
message(NOTICE "CXX: ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}")
message(NOTICE "\n")
