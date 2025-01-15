# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# This avoids running the linker.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#[=[
Check once if C++ source code can be compiled.

Options:

  CXXFLAGS - A list of additional flags to pass to the compiler.

For historical reasons, among the CMake `CMAKE_REQUIRED_*` variables that influence
`check_cxx_source_compiles()`, only `CMAKE_REQUIRED_FLAGS` is a string rather than
a list. Additionally, `target_compile_options()` also expects a list of options.

The `check_cxx_source_compiles_with_flags()` function handles this case and accepts
`CXXFLAGS` as a list, simplifying the code at the caller site.

]=]
function(check_cxx_source_compiles_with_flags source result_var)
  cmake_parse_arguments(PARSE_ARGV 2 _ "" "" CXXFLAGS)
  list(JOIN __CXXFLAGS " " CMAKE_REQUIRED_FLAGS)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("${source}" ${result_var})
  set(${result_var} ${${result_var}} PARENT_SCOPE)
endfunction()
