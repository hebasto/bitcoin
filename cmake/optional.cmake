# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Optional features and packages.

if(CCACHE)
  find_program(CCACHE_COMMAND ccache)
  if(CCACHE_COMMAND)
    if(MSVC)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        # ccache >= 4.8 requires compile batching turned off that is available since CMake 3.24.
        # See https://github.com/ccache/ccache/wiki/MS-Visual-Studio
        set(CCACHE ON)
        set(MSVC_CCACHE_WRAPPER_CONTENT "\"${CCACHE_COMMAND}\" \"${CMAKE_CXX_COMPILER}\"")
        set(MSVC_CCACHE_WRAPPER_FILENAME wrapped-cl.bat)
        file(WRITE ${CMAKE_BINARY_DIR}/${MSVC_CCACHE_WRAPPER_FILENAME} "${MSVC_CCACHE_WRAPPER_CONTENT} %*")
        list(APPEND CMAKE_VS_GLOBALS
          "CLToolExe=${MSVC_CCACHE_WRAPPER_FILENAME}"
          "CLToolPath=${CMAKE_BINARY_DIR}"
          "DebugInformationFormat=OldStyle"
        )
        set(CMAKE_VS_NO_COMPILE_BATCHING ON)
      elseif(CCACHE STREQUAL "AUTO")
        message(WARNING "ccache requested and found, but CMake >= 3.24 is required to use it properly. Disabling.\n"
                        "To skip ccache check, use \"-DCCACHE=OFF\".\n")
        set(CCACHE OFF)
      else()
        message(FATAL_ERROR "ccache requested and found, but CMake >= 3.24 is required to use it properly.")
      endif()
    else()
      set(CCACHE ON)
      list(APPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_COMMAND})
      list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_COMMAND})
    endif()
  elseif(CCACHE STREQUAL "AUTO")
    set(CCACHE OFF)
  else()
    message(FATAL_ERROR "ccache requested, but not found.")
  endif()
  mark_as_advanced(CCACHE_COMMAND)
endif()
