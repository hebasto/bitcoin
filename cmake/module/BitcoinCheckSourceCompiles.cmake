# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

#[=[
Check once if C++ source code can be compiled.

Options:

  CXXFLAGS - A list of additional flags to pass to the compiler.

]=]
function(bitcoin_check_cxx_source_compiles source var)
  cmake_parse_arguments(PARSE_ARGV 2 _ "" "" CXXFLAGS)
  # This avoids running the linker.
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  list(JOIN __CXXFLAGS " " CMAKE_REQUIRED_FLAGS)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${source}" ${var})
  set(${var} ${${var}} PARENT_SCOPE)
endfunction()
