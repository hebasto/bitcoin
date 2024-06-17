# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(MSVC AND WITH_CCACHE)
  # For integration of ccache and MSVC, please refer to
  # https://github.com/ccache/ccache/wiki/MS-Visual-Studio
  message("============ CCACHE_EXECUTABLE is ${CCACHE_EXECUTABLE}")
  find_program(CCACHE_EXECUTABLE ccache)
  message("============ CCACHE_EXECUTABLE is ${CCACHE_EXECUTABLE}")
  if(CCACHE_EXECUTABLE)
    file(COPY_FILE ${CCACHE_EXECUTABLE} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)
    list(APPEND CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "DebugInformationFormat=OldStyle"
    )
    set(CMAKE_VS_NO_COMPILE_BATCHING ON)
    # By default, Visual Studio generators uses /Zi, which is not compatible
    # with ccache, so tell Visual Studio to use /Z7 instead.
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
  else()
    set(WITH_CCACHE OFF)
  endif()
elseif(NOT MSVC)
  find_program(CCACHE_EXECUTABLE ccache)
  if(CCACHE_EXECUTABLE)
    execute_process(
      COMMAND readlink -f ${CMAKE_CXX_COMPILER}
      OUTPUT_VARIABLE compiler_resolved_link
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(CCACHE_EXECUTABLE STREQUAL compiler_resolved_link)
      set(WITH_CCACHE "ccache masquerades as the compiler")
    elseif(WITH_CCACHE)
      list(APPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
      list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
    endif()
  else()
    set(WITH_CCACHE OFF)
  endif()
endif()

mark_as_advanced(CCACHE_EXECUTABLE)
