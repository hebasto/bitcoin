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

if(WITH_USDT)
  find_package(USDT MODULE)
  if(USDT_FOUND)
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

if(WITH_GUI AND WITH_QRENCODE)
  if(VCPKG_TARGET_TRIPLET)
    # TODO: vcpkg fails to build libqrencode package due to
    #       the build error in its libiconv dependency.
    #       See: https://github.com/microsoft/vcpkg/issues/36924.
  else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(libqrencode IMPORTED_TARGET libqrencode)
  endif()
  if(TARGET PkgConfig::libqrencode)
    set_target_properties(PkgConfig::libqrencode PROPERTIES
      INTERFACE_COMPILE_DEFINITIONS USE_QRCODE
    )
    set(WITH_QRENCODE ON)
  elseif(WITH_QRENCODE STREQUAL "AUTO")
    set(WITH_QRENCODE OFF)
  else()
    message(FATAL_ERROR "libqrencode requested, but not found.")
  endif()
endif()

if(WITH_MULTIPROCESS)
  find_package(Libmultiprocess CONFIG)
  find_package(LibmultiprocessGen CONFIG)
  if(TARGET Libmultiprocess::multiprocess AND TARGET Libmultiprocess::mpgen)
    set(WITH_MULTIPROCESS ON)
  elseif(WITH_MULTIPROCESS STREQUAL "AUTO")
    set(WITH_MULTIPROCESS OFF)
  else()
    message(FATAL_ERROR "\"-DMULTIPROCESS=ON\" specified, but libmultiprocess library was not found.")
  endif()
endif()
