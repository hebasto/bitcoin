# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# We are able to build on Ubuntu Bionic.
# On the other hand, some useful features lack:
#  - 3.12: Object libraries can be linked to with target_link_libraries()
#  - 3.12: If the CONFIGURE_DEPENDS flag is specified, CMake will add logic to the main
#          build system check target to rerun the flagged GLOB commands at build time
#  - ...
cmake_minimum_required(VERSION 3.10)

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

# Configurable options.
option(BUILD_DAEMON "Build bitcoind" ON)
option(BUILD_CLI "Build bitcoin-cli" ON)
option(BUILD_BITCOINCONSENSUS_LIB "Build bitcoinconsensus shared library" ON)
option(BUILD_BITCOINKERNEL_LIB "Build experimental bitcoinkernel shared library" ON)
option(USE_CCACHE "Use ccache for building" ON)
option(REDUCE_EXPORTS "Attempt to reduce exported symbols in the resulting executables" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-pipe -O2)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
    add_compile_options(-Wno-psabi)
  endif()
  link_libraries(-Wl,-O2)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_compile_definitions(MAC_OSX OBJC_OLD_DISPATCH_PROTOTYPES=0)
  link_libraries(-Wl,-headerpad_max_install_names)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
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

if(USE_CCACHE)
  find_program(CCACHE ccache)
  if(CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
  endif()
endif()

if(REDUCE_EXPORTS)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
  link_libraries(-Wl,--exclude-libs,ALL)
endif()

include(cmake/subtree-crc32c.cmake)
include(cmake/subtree-leveldb.cmake)
include(cmake/subtree-minisketch.cmake)
include(cmake/subtree-secp256k1.cmake)
include(cmake/subtree-univalue.cmake)

include(FindPkgConfig)

if(BUILD_DAEMON OR BUILD_CLI OR BUILD_BITCOINKERNEL_LIB)
  # Find Boost headers only.
  find_package(Boost 1.64.0 REQUIRED)

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

if(BUILD_DAEMON OR BUILD_CLI)
  pkg_check_modules(libevent REQUIRED libevent>=2.1.8 IMPORTED_TARGET)
  if(WIN32)
    target_link_libraries(PkgConfig::libevent INTERFACE iphlpapi ws2_32)
  endif()
endif()

if(NOT WIN32 AND BUILD_DAEMON)
  pkg_check_modules(libevent_pthreads REQUIRED libevent_pthreads>=2.1.8 IMPORTED_TARGET)
endif()

add_subdirectory(src)

message("\n")
if(CMAKE_CROSSCOMPILING)
  message("Cross compiling for ${CMAKE_SYSTEM_NAME}, ${CMAKE_SYSTEM_PROCESSOR}")
endif()
message("CXX: ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}")
message("\n")
