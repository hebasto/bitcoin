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

set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(HAVE_CRC32C ON CACHE BOOL "" FORCE)
set(HAVE_SNAPPY OFF CACHE BOOL "" FORCE)
set(HAVE_TCMALLOC OFF CACHE BOOL "" FORCE)
# Disabling TSA warnings because leveldb simultaneously
# enables -Werror as a usage requirement.
set(HAVE_CLANG_THREAD_SAFETY OFF CACHE BOOL "" FORCE)
mark_as_advanced(
  LEVELDB_BUILD_TESTS
  LEVELDB_BUILD_BENCHMARKS
  LEVELDB_INSTALL
  HAVE_CRC32C
  HAVE_SNAPPY
  HAVE_TCMALLOC
  HAVE_CLANG_THREAD_SAFETY
)
add_subdirectory(leveldb)
# RTTI has been forcibly disabled in CMake configuration since leveldb v1.23.
# Commit: 69061b464ab1da287da9b7ffec1ed911b754403b
# Using leveldb::Logger as a base class to derive CBitcoinLevelDBLogger
# requires RTTI been enabled.
target_compile_options(leveldb
  PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-frtti>
    $<$<CXX_COMPILER_ID:MSVC>:/wd4244 /wd4267 /wd4996>
)
set_property(
  TARGET leveldb leveldbutil
  PROPERTY EXCLUDE_FROM_ALL TRUE
)

configure_file(../cmake/bitcoin-config.h.in config/bitcoin-config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/src)
add_definitions(-DHAVE_CONFIG_H)

add_subdirectory(univalue)
