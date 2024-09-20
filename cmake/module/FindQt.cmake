# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

#[=======================================================================[
FindQt
------

Finds the Qt 6 headers and libraries.

This is a wrapper around find_package() command that:
 - facilitates searching in various build environments
 - prints a standard log message

#]=======================================================================]

set(_qt_homebrew_prefix)
if(CMAKE_HOST_APPLE)
  find_program(HOMEBREW_EXECUTABLE brew)
  if(HOMEBREW_EXECUTABLE)
    execute_process(
      COMMAND ${HOMEBREW_EXECUTABLE} --prefix qt
      OUTPUT_VARIABLE _qt_homebrew_prefix
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

# Save CMAKE_FIND_ROOT_PATH_MODE_LIBRARY state.
unset(_qt_find_root_path_mode_library_saved)
if(DEFINED CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
  set(_qt_find_root_path_mode_library_saved ${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
endif()

# The Qt config files internally use find_library() calls for all
# dependencies to ensure their availability. In turn, the find_library()
# inspects the well-known locations on the file system; therefore, it must
# be able to find platform-specific system libraries, for example:
# /usr/x86_64-w64-mingw32/lib/libm.a or /usr/arm-linux-gnueabihf/lib/libm.a.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)

find_package(Qt6 ${Qt_FIND_VERSION} NO_MODULE QUIET
  COMPONENTS ${Qt_FIND_COMPONENTS}
  HINTS ${_qt_homebrew_prefix}
  PATH_SUFFIXES Qt6  # Required on OpenBSD systems.
)
unset(_qt_homebrew_prefix)

# Restore CMAKE_FIND_ROOT_PATH_MODE_LIBRARY state.
if(DEFINED _qt_find_root_path_mode_library_saved)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ${_qt_find_root_path_mode_library_saved})
  unset(_qt_find_root_path_mode_library_saved)
else()
  unset(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qt
  REQUIRED_VARS Qt6_DIR Qt6_FOUND
  VERSION_VAR Qt6_VERSION
)

foreach(component IN LISTS Qt_FIND_COMPONENTS ITEMS "")
  mark_as_advanced(Qt6${component}_DIR)
endforeach()
