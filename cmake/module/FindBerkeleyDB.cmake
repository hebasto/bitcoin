# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

if(BREW_COMMAND)
  #[=[
  The Homebrew package manager installs the berkeley-db* packages as
  "keg-only", which means they are not symlinked into the default prefix.
  To find such a package, the find_path() and find_library() commands
  need additional path hints that are computed by Homebrew itself.
  ]=]
  execute_process(
    COMMAND ${BREW_COMMAND} --prefix berkeley-db@4
    OUTPUT_VARIABLE bdb4_brew_prefix
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND ${BREW_COMMAND} --prefix berkeley-db@5
    OUTPUT_VARIABLE bdb5_brew_prefix
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND ${BREW_COMMAND} --prefix berkeley-db
    OUTPUT_VARIABLE bdb_brew_prefix
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(BerkeleyDB_homebrew_include_hints ${bdb4_brew_prefix}/include ${bdb5_brew_prefix}/include ${bdb_brew_prefix}/include)
  set(BerkeleyDB_homebrew_lib_hints ${bdb4_brew_prefix}/lib ${bdb5_brew_prefix}/lib ${bdb_brew_prefix}/lib)
endif()

find_path(BerkeleyDB_INCLUDE_DIR
  NAMES db.h
  HINTS ${BerkeleyDB_homebrew_include_hints}
  PATH_SUFFIXES 4.8 48 4 db4 5.3 5 db5
)
unset(BerkeleyDB_homebrew_include_hints)

if(MSVC)
  cmake_path(GET BerkeleyDB_INCLUDE_DIR PARENT_PATH BerkeleyDB_IMPORTED_PATH)
  find_library(BerkeleyDB_LIBRARY_DEBUG
    NAMES libdb48 PATHS ${BerkeleyDB_IMPORTED_PATH}/debug/lib
    NO_DEFAULT_PATH
  )
  find_library(BerkeleyDB_LIBRARY_RELEASE
    NAMES libdb48 PATHS ${BerkeleyDB_IMPORTED_PATH}/lib
    NO_DEFAULT_PATH
  )
  if(BerkeleyDB_LIBRARY_DEBUG OR BerkeleyDB_LIBRARY_RELEASE)
    set(BerkeleyDB_required BerkeleyDB_IMPORTED_PATH)
  endif()
else()
  find_library(BerkeleyDB_LIBRARY
    NAMES db_cxx-4.8 db4_cxx db48 db_cxx-5.3 db_cxx-5 db_cxx
    HINTS ${BerkeleyDB_homebrew_lib_hints}
  )
  unset(BerkeleyDB_homebrew_lib_hints)
  set(BerkeleyDB_required BerkeleyDB_LIBRARY)
endif()

if(BerkeleyDB_INCLUDE_DIR AND BerkeleyDB_required)
  file(
    STRINGS "${BerkeleyDB_INCLUDE_DIR}/db.h" version_strings
    REGEX ".*DB_VERSION_(MAJOR|MINOR)[ \t]+[0-9]+.*"
  )
  string(REGEX REPLACE ".*DB_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" BerkeleyDB_VERSION_MAJOR "${version_strings}")
  string(REGEX REPLACE ".*DB_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" BerkeleyDB_VERSION_MINOR "${version_strings}")
  set(BerkeleyDB_VERSION ${BerkeleyDB_VERSION_MAJOR}.${BerkeleyDB_VERSION_MINOR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BerkeleyDB
  REQUIRED_VARS ${BerkeleyDB_required} BerkeleyDB_INCLUDE_DIR
  VERSION_VAR BerkeleyDB_VERSION
)

if(BerkeleyDB_FOUND AND NOT TARGET BerkeleyDB::BerkeleyDB)
  add_library(BerkeleyDB::BerkeleyDB UNKNOWN IMPORTED)
  set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${BerkeleyDB_INCLUDE_DIR}"
  )
  if(MSVC)
    if(BerkeleyDB_LIBRARY_DEBUG)
      set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
        IMPORTED_LOCATION_DEBUG "${BerkeleyDB_LIBRARY_DEBUG}"
      )
    endif()
    if(BerkeleyDB_LIBRARY_RELEASE)
      set_property(TARGET BerkeleyDB::BerkeleyDB APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(BerkeleyDB::BerkeleyDB PROPERTIES
        IMPORTED_LOCATION_RELEASE "${BerkeleyDB_LIBRARY_RELEASE}"
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
  BerkeleyDB_LIBRARY_DEBUG
  BerkeleyDB_LIBRARY_RELEASE
)
