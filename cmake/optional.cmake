# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Optional features and packages.

if(WITH_CCACHE)
  set(ccache_hints)
  if(MSVC AND EXISTS "$ENV{ChocolateyInstall}")
    # Bypass a shim executable provided by Chocolatey.
    # See https://docs.chocolatey.org/en-us/features/shim
    file(GLOB ccache_hints "$ENV{ChocolateyInstall}/lib/ccache/tools/ccache-*")
  endif()
  find_program(CCACHE_COMMAND ccache HINTS ${ccache_hints})
  unset(ccache_hints)

  if(CCACHE_COMMAND)
    if(MSVC)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        # ccache >= 4.8 requires compile batching turned off that is available since CMake 3.24.
        # See https://github.com/ccache/ccache/wiki/MS-Visual-Studio
        set(WITH_CCACHE ON)
        file(COPY_FILE ${CCACHE_COMMAND} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)
        list(APPEND CMAKE_VS_GLOBALS
          "CLToolExe=cl.exe"
          "CLToolPath=${CMAKE_BINARY_DIR}"
          "DebugInformationFormat=OldStyle"
        )
        set(CMAKE_VS_NO_COMPILE_BATCHING ON)
        # By default Visual Studio generators will use /Zi which is not compatible
        # with ccache, so tell Visual Studio to use /Z7 instead.
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
      elseif(WITH_CCACHE STREQUAL "AUTO")
        message(WARNING "ccache requested and found, but CMake >= 3.24 is required to use it properly. Disabling.\n"
                        "To skip ccache check, use \"-DWITH_CCACHE=OFF\".\n")
        set(WITH_CCACHE OFF)
      else()
        message(FATAL_ERROR "ccache requested and found, but CMake >= 3.24 is required to use it properly.")
      endif()
    else()
      set(WITH_CCACHE ON)
      list(APPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_COMMAND})
      list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_COMMAND})
    endif()
  elseif(WITH_CCACHE STREQUAL "AUTO")
    set(WITH_CCACHE OFF)
  else()
    message(FATAL_ERROR "ccache requested, but not found.")
  endif()
  mark_as_advanced(CCACHE_COMMAND)
endif()
