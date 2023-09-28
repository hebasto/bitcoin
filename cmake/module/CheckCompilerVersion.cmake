# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# See https://github.com/bitcoin/bitcoin/blob/master/doc/dependencies.md
function(check_compiler_version)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(compiler_minimum_required "12.0")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(compiler_minimum_required "10.0")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(compiler_minimum_required "9.1")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(compiler_minimum_required "19.30")
  endif()

  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS compiler_minimum_required)
    message(FATAL_ERROR "The minimum required compiler version is ${compiler_minimum_required}")
  endif()
endfunction()
