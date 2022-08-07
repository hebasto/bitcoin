# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# GCC 8.x (libstdc++) requires -lstdc++fs
# Clang 8.x (libc++) requires -lc++fs

include(CheckCXXSourceCompiles)

set(check_filesystem_code "
#include <filesystem>

int main()
{
  (void)std::filesystem::current_path().root_name();
}
")
check_cxx_source_compiles("${check_filesystem_code}" FILESYSTEM_WORKS_WITHOUT_LINK)
if(NOT FILESYSTEM_WORKS_WITHOUT_LINK)
  set(TEMP_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
  check_cxx_source_compiles("${check_filesystem_code}" FILESYSTEM_NEEDS_LINK_TO_STDCXXFS)
  if(FILESYSTEM_NEEDS_LINK_TO_STDCXXFS)
    link_libraries(stdc++fs)
  else()
    set(CMAKE_REQUIRED_LIBRARIES c++fs)
    check_cxx_source_compiles("${check_filesystem_code}" FILESYSTEM_NEEDS_LINK_TO_CXXFS)
    if(FILESYSTEM_NEEDS_LINK_TO_CXXFS)
      link_libraries(c++fs)
    else()
      message(FATAL_ERROR "Cannot figure out how to use std::filesystem.")
    endif()
  endif()
  set(CMAKE_REQUIRED_LIBRARIES ${TEMP_CMAKE_REQUIRED_LIBRARIES})
endif()
