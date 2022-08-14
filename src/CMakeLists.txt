# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set(CRC32C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(CRC32C_USE_GLOG OFF CACHE BOOL "" FORCE)
set(CRC32C_INSTALL OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
mark_as_advanced(
  CRC32C_BUILD_TESTS
  CRC32C_BUILD_BENCHMARKS
  CRC32C_USE_GLOG
  CRC32C_INSTALL
)
add_subdirectory(crc32c)
set_property(
  TARGET crc32c_arm64 crc32c_sse42
  APPEND
  PROPERTY COMPILE_DEFINITIONS CRC32C_HAVE_CONFIG_H
)
set_property(
  TARGET crc32c crc32c_arm64 crc32c_sse42
  PROPERTY EXCLUDE_FROM_ALL TRUE
)

configure_file(../cmake/bitcoin-config.h.in config/bitcoin-config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/src)
add_definitions(-DHAVE_CONFIG_H)
