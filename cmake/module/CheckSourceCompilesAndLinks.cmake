# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# This avoids running the linker.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

macro(check_cxx_source_compiles_with_flags flags source)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_FLAGS ${flags})
  list(JOIN CMAKE_REQUIRED_FLAGS " " CMAKE_REQUIRED_FLAGS)
  check_cxx_source_compiles("${source}" ${ARGN})
  cmake_pop_check_state()
endmacro()
