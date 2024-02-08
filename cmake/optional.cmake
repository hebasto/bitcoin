# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Optional features and packages.

if(CCACHE)
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
        set(CCACHE ON)
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

if(WITH_NATPMP)
  find_package(NATPMP MODULE)
  if(NATPMP_FOUND)
    set(WITH_NATPMP ON)
  elseif(WITH_NATPMP STREQUAL "AUTO")
    message(WARNING "libnatpmp not found, disabling.\n"
                    "To skip libnatpmp check, use \"-DWITH_NATPMP=OFF\".\n")
    set(WITH_NATPMP OFF)
  else()
    message(FATAL_ERROR "libnatpmp requested, but not found.")
  endif()
endif()

if(WITH_MINIUPNPC)
  find_package(MiniUPnPc MODULE)
  if(MiniUPnPc_FOUND)
    set(WITH_MINIUPNPC ON)
  elseif(WITH_MINIUPNPC STREQUAL "AUTO")
    message(WARNING "libminiupnpc not found, disabling.\n"
                    "To skip libminiupnpc check, use \"-DWITH_MINIUPNPC=OFF\".\n")
    set(WITH_MINIUPNPC OFF)
  else()
    message(FATAL_ERROR "libminiupnpc requested, but not found.")
  endif()
endif()

if(WITH_ZMQ)
  if(MSVC)
    find_package(ZeroMQ CONFIG)
  else()
    # The ZeroMQ project has provided config files since v4.2.2.
    # TODO: Switch to find_package(ZeroMQ) at some point in the future.
    include(CrossPkgConfig)
    cross_pkg_check_modules(libzmq IMPORTED_TARGET libzmq>=4)
    if(libzmq_FOUND AND TARGET PkgConfig::libzmq)
      target_compile_definitions(PkgConfig::libzmq INTERFACE
        $<$<PLATFORM_ID:Windows>:ZMQ_STATIC>
      )
      target_link_libraries(PkgConfig::libzmq INTERFACE
        $<$<PLATFORM_ID:Windows>:iphlpapi;ws2_32>
      )
    endif()
  endif()
  if(TARGET libzmq OR TARGET PkgConfig::libzmq)
    set(WITH_ZMQ ON)
  elseif(WITH_ZMQ STREQUAL "AUTO")
    message(WARNING "libzmq not found, disabling.\n"
                    "To skip libzmq check, use \"-DWITH_ZMQ=OFF\".\n")
    set(WITH_ZMQ OFF)
  else()
    message(FATAL_ERROR "libzmq requested, but not found.")
  endif()
endif()

if(WITH_USDT)
  find_path(SystemTap_INCLUDE_DIR
    NAMES sys/sdt.h
  )
  mark_as_advanced(SystemTap_INCLUDE_DIR)

  if(SystemTap_INCLUDE_DIR)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)

    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_INCLUDES ${SystemTap_INCLUDE_DIR})
    check_cxx_source_compiles("
      #include <sys/sdt.h>

      int main()
      {
        DTRACE_PROBE(context, event);
        int a, b, c, d, e, f, g;
        DTRACE_PROBE7(context, event, a, b, c, d, e, f, g);
      }
      " HAVE_USDT_H
    )

    cmake_pop_check_state()
  endif()

  if(HAVE_USDT_H)
    target_include_directories(core_interface INTERFACE
      ${SystemTap_INCLUDE_DIR}
    )
    set(ENABLE_TRACING TRUE)
    set(WITH_USDT ON)
  elseif(WITH_USDT STREQUAL "AUTO")
    set(WITH_USDT OFF)
  else()
    message(FATAL_ERROR "sys/sdt.h requested, but not found.")
  endif()
endif()

if(ENABLE_WALLET)
  if(WITH_SQLITE)
    if(VCPKG_TARGET_TRIPLET)
      # Use of the `unofficial::` namespace is a vcpkg package manager convention.
      find_package(unofficial-sqlite3 CONFIG)
    else()
      find_package(SQLite3 3.7.17)
    endif()
    if(TARGET unofficial::sqlite3::sqlite3 OR TARGET SQLite::SQLite3)
      set(WITH_SQLITE ON)
      set(USE_SQLITE ON)
    elseif(WITH_SQLITE STREQUAL "AUTO")
      set(WITH_SQLITE OFF)
    else()
      message(FATAL_ERROR "SQLite requested, but not found.")
    endif()
  endif()

  if(WITH_BDB)
    find_package(BerkeleyDB 4.8 MODULE)
    if(BerkeleyDB_FOUND)
      set(WITH_BDB ON)
      set(USE_BDB ON)
      if(NOT BerkeleyDB_VERSION VERSION_EQUAL 4.8)
        message(WARNING "Found Berkeley DB (BDB) other than 4.8.")
        if(WARN_INCOMPATIBLE_BDB)
          message(WARNING "BDB (legacy) wallets opened by this build would not be portable!\n"
                          "If this is intended, pass \"-DWARN_INCOMPATIBLE_BDB=OFF\".\n"
                          "Passing \"-DWITH_BDB=OFF\" will suppress this warning.\n")
        else()
          message(WARNING "BDB (legacy) wallets opened by this build will not be portable!")
        endif()
      endif()
    else()
      message(WARNING "Berkeley DB (BDB) required for legacy wallet support, but not found.\n"
                      "Passing \"-DWITH_BDB=OFF\" will suppress this warning.\n")
      set(WITH_BDB OFF)
    endif()
  endif()
else()
  set(WITH_SQLITE OFF)
  set(WITH_BDB OFF)
endif()
