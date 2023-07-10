# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# IMPORTANT: Changes which affect binary results may not be quietly gated
#            by CMake version.
#
# Ubuntu 20.04 LTS Focal Fossa, https://wiki.ubuntu.com/Releases, EOSS in April 2025:
#  - CMake 3.16.3, https://packages.ubuntu.com/focal/cmake
#
# Centos Stream 8, EOL in May 2024:
#  - CMake 3.20.2, http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/
#
# All policies known to the running version of CMake and introduced
# in the 3.28 version or earlier will be set to use NEW behavior.
# All policies introduced in later versions will be unset.
# See: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
cmake_minimum_required(VERSION 3.16...3.28)

project("Bitcoin Core"
  VERSION 25.99.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX ASM
)

set(CLIENT_VERSION_IS_RELEASE "false")
set(COPYRIGHT_YEAR "2023")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_FINAL "The ${PROJECT_NAME} developers")
set(PACKAGE_BUGREPORT "https://github.com/bitcoin/bitcoin/issues")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/module)

# Configurable options.
# When adding a new option, end the <help_text> with a full stop for consistency.
include(CMakeDependentOption)
cmake_dependent_option(CXX20 "Enable compilation in C++20 mode." OFF "NOT MSVC" ON)

if(CXX20)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(configure_warnings)

include(CheckSourceCompilesAndLinks)
include(cmake/introspection.cmake)

add_subdirectory(src)

message("\n")
message("Configure summary")
message("=================")
get_directory_property(definitions COMPILE_DEFINITIONS)
string(REPLACE ";" " " definitions "${definitions}")
message("Preprocessor defined macros ........... ${definitions}")
message("C compiler ............................ ${CMAKE_C_COMPILER}")
message("CFLAGS ................................ ${CMAKE_C_FLAGS}")
message("C++ compiler .......................... ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS .............................. ${CMAKE_CXX_FLAGS}")
get_directory_property(common_compile_options COMPILE_OPTIONS)
string(REPLACE ";" " " common_compile_options "${common_compile_options}")
message("Common compile options ................ ${common_compile_options}")
get_directory_property(common_link_options LINK_OPTIONS)
string(REPLACE ";" " " common_link_options "${common_link_options}")
message("Common link options ................... ${common_link_options}")
message("Linker flags for executables .......... ${CMAKE_EXE_LINKER_FLAGS}")
message("Linker flags for shared libraries ..... ${CMAKE_SHARED_LINKER_FLAGS}")
if(DEFINED CMAKE_BUILD_TYPE)
  message("Build type:")
  message(" - CMAKE_BUILD_TYPE ................... ${CMAKE_BUILD_TYPE}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" build_type)
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_${build_type}}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_${build_type}}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_${build_type}}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_${build_type}}")
else()
  message("Available configurations .............. ${CMAKE_CONFIGURATION_TYPES}")
  message("Debug configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_DEBUG}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_DEBUG}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
  message("Release configuration:")
  message(" - CFLAGS ............................. ${CMAKE_C_FLAGS_RELEASE}")
  message(" - CXXFLAGS ........................... ${CMAKE_CXX_FLAGS_RELEASE}")
  message(" - LDFLAGS for executables ............ ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  message(" - LDFLAGS for shared libraries ....... ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
endif()
message("\n")
if(configure_warnings)
    message("  ******\n")
    foreach(warning IN LISTS configure_warnings)
      message(WARNING "${warning}")
    endforeach()
    message("  ******\n")
endif()

# We want all build properties to be encapsulated properly.
get_directory_property(global_compile_definitions COMPILE_DEFINITIONS)
if(global_compile_definitions)
  message(AUTHOR_WARNING "The directory's COMPILE_DEFINITIONS property is not empty: ${global_compile_definitions}")
endif()
get_directory_property(global_compile_options COMPILE_OPTIONS)
if(global_compile_options)
  message(AUTHOR_WARNING "The directory's COMPILE_OPTIONS property is not empty: ${global_compile_options}")
endif()
get_directory_property(global_link_options LINK_OPTIONS)
if(global_link_options)
  message(AUTHOR_WARNING "The directory's LINK_OPTIONS property is not empty: ${global_link_options}")
endif()
