# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(WITH_GUI "AUTO" CACHE STRING "Build GUI ([AUTO], Qt5, OFF)")
set(with_gui_values AUTO Qt5 OFF)
if(NOT WITH_GUI IN_LIST with_gui_values)
  message(FATAL_ERROR "WITH_GUI value is \"${WITH_GUI}\", but must be one of \"AUTO\", \"Qt5\" or \"OFF\".")
endif()

if(WITH_GUI)
  set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
  set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

  if(BREW_COMMAND)
    execute_process(
      COMMAND ${BREW_COMMAND} --prefix qt@5
      OUTPUT_VARIABLE qt5_brew_prefix
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  if(WITH_GUI STREQUAL "AUTO")
    # The PATH_SUFFIXES option is required on OpenBSD systems.
    find_package(QT NAMES Qt5
      COMPONENTS Core
      HINTS ${qt5_brew_prefix}
      PATH_SUFFIXES Qt5
    )
    if(QT_FOUND)
      set(WITH_GUI Qt${QT_VERSION_MAJOR})
      if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        enable_language(OBJCXX)
        set(CMAKE_OBJCXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        set(CMAKE_OBJCXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_OBJCXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        set(CMAKE_OBJCXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
      endif()
    else()
      message(WARNING "Qt not found, disabling.\n"
                      "To skip this warning check, use \"-DWITH_GUI=OFF\".\n")
      set(WITH_GUI OFF)
      set(BUILD_GUI_TESTS OFF)
    endif()
  endif()
endif()
