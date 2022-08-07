# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# We are able to build on Ubuntu Bionic.
# On the other hand, some useful features lack:
#  - 3.11: add_library() and add_executable() commands can now be called without any sources
#          and will not complain as long as sources are added later via the target_sources()
#          command.
#  - 3.11: The target_*() commands learned to set the INTERFACE properties on Imported Targets.
#  - 3.12: The add_compile_definitions() command was added to set preprocessor definitions
#          at directory level. This supersedes add_definitions().
#  - 3.12: If the CONFIGURE_DEPENDS flag is specified, CMake will add logic to the main
#          build system check target to rerun the flagged GLOB commands at build time.
#  - 3.12: Object libraries can be linked to with target_link_libraries().
#  - 3.12: A new $<TARGET_NAME_IF_EXISTS:...> generator expression has been added.
#  - 3.13: The target_link_options() command was created to specify link options for targets
#          and their dependents.
#  - 3.13: The target_link_libraries() command may now be called to modify targets created
#          outside the current directory.
#  - 3.14: The FindSQLite3 module was added to find the SQLite v3.x library.
#  - 3.15: A new imported target Boost::headers is now defined (same as Boost::boost).
#  - 3.18: The CheckLinkerFlag module has been added to provide a facility to check validity
#          of link flags.
cmake_minimum_required(VERSION 3.10)

project("Bitcoin Core"
  VERSION 23.0.0
  DESCRIPTION "Bitcoin client software"
  LANGUAGES CXX C ASM
)

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})
set(CLIENT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CLIENT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CLIENT_VERSION_BUILD ${PROJECT_VERSION_PATCH})
set(CLIENT_VERSION_IS_RELEASE "false")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")
set(PACKAGE_URL "https://bitcoincore.org/")
set(COPYRIGHT_YEAR "2022")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_SUBSTITUTION ${PROJECT_NAME})
set(COPYRIGHT_HOLDERS_FINAL "The Bitcoin Core developers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configurable options.
option(REDUCE_EXPORTS "Attempt to reduce exported symbols in the resulting executables" OFF)

include(cmake/try_append_cxxflag.cmake)
# Define custom "RelWithAsserts" build type
set(release_cxx_flags "")
try_append_cxxflag(release_cxx_flags "-pipe")
try_append_cxxflag(release_cxx_flags "-O2")
try_append_cxxflag(release_cxx_flags "-g")
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  # All versions of gcc that we commonly use for building are subject to bug
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348. To work around that, set
  # -fstack-reuse=none for all gcc builds.
  try_append_cxxflag(release_cxx_flags "-fstack-reuse=none")
endif()
set(CMAKE_CXX_FLAGS_RELWITHASSERTS "${release_cxx_flags}" CACHE STRING
  "Flags used by the C++ compiler during \"RelWithAsserts\" builds."
  FORCE
)
# Define custom "RelHardened" build type
set(hardened_cxx_flags "")
try_append_cxxflag(hardened_cxx_flags "-Wstack-protector")
try_append_cxxflag(hardened_cxx_flags "-fstack-protector-all")
try_append_cxxflag(hardened_cxx_flags "-fcf-protection=full")
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  try_append_cxxflag(hardened_cxx_flags "-fstack-clash-protection")
endif()
set(CMAKE_CXX_FLAGS_RELHARDENED "${release_cxx_flags} ${hardened_cxx_flags}" CACHE STRING
  "Flags used by the C++ compiler during \"RelHardened\" builds."
  FORCE
)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  set(hardened_linker_flags "-Wl,-z,relro -Wl,-z,now -Wl,-z,separate-code")
elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
  set(hardened_linker_flags "-Wl,--enable-reloc-section -Wl,--dynamicbase -Wl,--nxcompat -Wl,--high-entropy-va")
endif()
set(CMAKE_EXE_LINKER_FLAGS_RELHARDENED "${hardened_linker_flags}" CACHE STRING
  "Flags used for linking binaries during \"RelHardened\" builds."
  FORCE
)
set(CMAKE_SHARED_LINKER_FLAGS_RELHARDENED "${hardened_linker_flags}" CACHE STRING
  "Flags used by the shared libraries linker during \"RelHardened\" builds."
  FORCE
)
mark_as_advanced(
  CMAKE_CXX_FLAGS_RELWITHASSERTS
  CMAKE_CXX_FLAGS_RELHARDENED
  CMAKE_EXE_LINKER_FLAGS_RELHARDENED
  CMAKE_SHARED_LINKER_FLAGS_RELHARDENED
)
get_property(cached_cmake_build_type CACHE CMAKE_BUILD_TYPE PROPERTY TYPE)
if(cached_cmake_build_type)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
    STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo" "RelWithAsserts" "RelHardened"
  )
endif()
set(default_build_type "RelHardened")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to \"${default_build_type}\" as none was specified")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(REDUCE_EXPORTS)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    link_libraries(-Wl,--exclude-libs,ALL)
  endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
    add_compile_options(-Wno-psabi)
  endif()
  link_libraries(-Wl,-O2)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_definitions(-DMAC_OSX -DOBJC_OLD_DISPATCH_PROTOTYPES=0)
  link_libraries(-Wl,-headerpad_max_install_names)
endif()

add_subdirectory(src)

message("\n")
message("Configure summary")
message("=================")
if(cached_cmake_build_type)
  message("Build type ......................... ${CMAKE_BUILD_TYPE}")
else()
  message("Available configurations ........... ${CMAKE_CONFIGURATION_TYPES}")
endif()
get_directory_property(definitions COMPILE_DEFINITIONS)
message("Preprocessor defined macros ........ ${definitions}")
message("C++ compiler ....................... ${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}")
get_directory_property(compile_options COMPILE_OPTIONS)
message("Compile options .................... ${compile_options}")
message("\n")
