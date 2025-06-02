# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(add_minisketch subdir)
  message("")
  message("Configuring minisketch subtree...")
  set(CMAKE_EXPORT_COMPILE_COMMANDS OFF)
  set(MINISKETCH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(MINISKETCH_FIELDS 32 CACHE STRING "" FORCE)
  add_subdirectory(${subdir} EXCLUDE_FROM_ALL)
  target_link_libraries(minisketch
    PRIVATE
      core_interface
  )
endfunction()
