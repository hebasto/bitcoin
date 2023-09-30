# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#[=======================================================================[
FindBerkeleyDB
--------------

Finds the Berkeley DB headers and library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides imported target ``BerkeleyDB::BerkeleyDB``, if
Berkeley DB has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``BerkeleyDB_FOUND``
  "True" if Berkeley DB found.

``BerkeleyDB_VERSION``
  The MAJOR.MINOR version of Berkeley DB found.

``BerkeleyDB_INCLUDE_DIRS``
  Include directories needed to use Berkeley DB.

``BerkeleyDB_LIBRARIES``
  Libraries needed to link to Berkeley DB.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``BerkeleyDB_INCLUDE_DIR``
  The directory containing ``db.h`` and ``db_cxx.h``.

``BerkeleyDB_LIBRARY``
  The path to the Berkeley DB library.

#]=======================================================================]

if(BREW_COMMAND)
  #[[
  The Homebrew package manager installs the berkeley-db* packages as
  "keg-only", which means they are not symlinked into the default prefix.
  To find such a package, the find_path() and find_library() commands
  need additional path hints that are computed by Homebrew itself.
  #]]
  list(APPEND _BerkeleyDB_homebrew_include_hints)
  list(APPEND _BerkeleyDB_homebrew_lib_hints)
  foreach(_suffix IN ITEMS db@4 db@5 db)
    execute_process(
      COMMAND ${BREW_COMMAND} --prefix berkeley-${_suffix}
      OUTPUT_VARIABLE _BerkeleyDB_homebrew_prefix_${_suffix}
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    list(APPEND _BerkeleyDB_homebrew_include_hints ${_BerkeleyDB_homebrew_prefix_${_suffix}}/include)
    list(APPEND _BerkeleyDB_homebrew_lib_hints ${_BerkeleyDB_homebrew_prefix_${_suffix}}/lib)
    unset(_BerkeleyDB_homebrew_prefix_${_suffix})
  endforeach()
  if(CMAKE_VERSION VERSION_LESS 3.21)
    unset(_suffix)
  endif()
endif()

find_path(BerkeleyDB_INCLUDE_DIR
  NAMES db_cxx.h
  HINTS ${_BerkeleyDB_homebrew_include_hints}
  PATH_SUFFIXES 4.8 48 4 db4 5.3 5 db5
)
unset(_BerkeleyDB_homebrew_include_hints)

if(MSVC AND NOT BerkeleyDB_LIBRARY)
  if(VCPKG_TARGET_TRIPLET)
    #[[
    The vcpkg package manager installs the berkeleydb package with the same name
    of release and debug libraries. Therefore, the default search paths set by
    vcpkg's toolchain file cannot be used to search libraries as the debug one
    will always be found.
    #]]
    set(CMAKE_FIND_USE_CMAKE_PATH FALSE)

    #[[
    We assume that the installation directory has the following structure:
      ../debug/lib
      ../include
      ../lib
    #]]
    cmake_path(GET BerkeleyDB_INCLUDE_DIR PARENT_PATH _BerkeleyDB_installed_path)
  endif()

  find_library(BerkeleyDB_LIBRARY_RELEASE
    NAMES libdb48
    HINTS ${_BerkeleyDB_installed_path}
    PATH_SUFFIXES lib
  )
  mark_as_advanced(BerkeleyDB_LIBRARY_RELEASE)

  find_library(BerkeleyDB_LIBRARY_DEBUG
    NAMES libdb48
    HINTS ${_BerkeleyDB_installed_path}
    PATH_SUFFIXES debug/lib
  )
  mark_as_advanced(BerkeleyDB_LIBRARY_DEBUG)
  unset(_BerkeleyDB_installed_path)
  unset(CMAKE_FIND_USE_CMAKE_PATH)

  include(SelectLibraryConfigurations)
  select_library_configurations(BerkeleyDB)
else()
  find_library(BerkeleyDB_LIBRARY
    NAMES db_cxx-4.8 db4_cxx db48 db_cxx-5.3 db_cxx-5 db_cxx
    HINTS ${_BerkeleyDB_homebrew_lib_hints}
  )
  unset(_BerkeleyDB_homebrew_lib_hints)
endif()

if(BerkeleyDB_INCLUDE_DIR)
  file(STRINGS "${BerkeleyDB_INCLUDE_DIR}/db.h" _BerkeleyDB_version_strings REGEX "^#define[\t ]+DB_VERSION_(MAJOR|MINOR|PATCH)[ \t]+[0-9]+.*")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_major "${_BerkeleyDB_version_strings}")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_minor "${_BerkeleyDB_version_strings}")
  string(REGEX REPLACE ".*#define[\t ]+DB_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" _BerkeleyDB_version_patch "${_BerkeleyDB_version_strings}")
  unset(_BerkeleyDB_version_strings)
  # The MAJOR.MINOR.PATCH version will be logged in the following find_package_handle_standard_args() command.
  set(_BerkeleyDB_full_version ${_BerkeleyDB_version_major}.${_BerkeleyDB_version_minor}.${_BerkeleyDB_version_patch})
  set(BerkeleyDB_VERSION ${_BerkeleyDB_version_major}.${_BerkeleyDB_version_minor})
  unset(_BerkeleyDB_version_major)
  unset(_BerkeleyDB_version_minor)
  unset(_BerkeleyDB_version_patch)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BerkeleyDB
  REQUIRED_VARS BerkeleyDB_LIBRARY BerkeleyDB_INCLUDE_DIR
  VERSION_VAR _BerkeleyDB_full_version
)
unset(_BerkeleyDB_full_version)

if(BerkeleyDB_FOUND AND NOT TARGET BerkeleyDB::BerkeleyDB)
  add_library(BerkeleyDB::BerkeleyDB UNKNOWN IMPORTED)
  set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BerkeleyDB_INCLUDE_DIR}"
  )
  if(MSVC)
    if(BerkeleyDB_LIBRARY_RELEASE)
      set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
        IMPORTED_LOCATION_RELEASE "${BerkeleyDB_LIBRARY_RELEASE}"
      )
    endif()
    if(BerkeleyDB_LIBRARY_DEBUG)
      set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
        IMPORTED_LOCATION_DEBUG "${BerkeleyDB_LIBRARY_DEBUG}"
      )
    endif()
  else()
    set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
      IMPORTED_LOCATION "${BerkeleyDB_LIBRARY}"
    )
  endif()
endif()

mark_as_advanced(
  BerkeleyDB_INCLUDE_DIR
  BerkeleyDB_LIBRARY
)
