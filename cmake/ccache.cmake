# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# The <LANG>_COMPILER_LAUNCHER target property, used to integrate
# Ccache, is supported only by the Makefiles and Ninja generators.
if(CMAKE_GENERATOR MATCHES "Makefiles|Ninja")
  find_program(CCACHE_EXECUTABLE ccache)
  if(CCACHE_EXECUTABLE)
    execute_process(
      COMMAND readlink -f ${CMAKE_CXX_COMPILER}
      OUTPUT_VARIABLE compiler_resolved_link
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(CCACHE_EXECUTABLE STREQUAL compiler_resolved_link AND NOT WITH_CCACHE)
      list(APPEND configure_warnings
        "Disabling ccache was attempted using -DWITH_CCACHE=${WITH_CCACHE}, but ccache masquerades as the compiler."
      )
      set(WITH_CCACHE ON)
    elseif(WITH_CCACHE)
      foreach(lang IN ITEMS C CXX OBJCXX)
        set(CMAKE_${lang}_COMPILER_LAUNCHER
          ${CMAKE_COMMAND} -E env CCACHE_NOHASHDIR=1 ${CCACHE_EXECUTABLE}
        )
      endforeach()
    endif()
  else()
    set(WITH_CCACHE OFF)
  endif()
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
