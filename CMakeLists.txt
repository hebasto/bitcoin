# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

cmake_minimum_required(VERSION 3.13)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.15)
  # MSVC runtime library flags are selected by the CMAKE_MSVC_RUNTIME_LIBRARY abstraction.
  cmake_policy(SET CMP0091 NEW)
  # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default.
  cmake_policy(SET CMP0092 NEW)
endif()
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
  # MSVC RTTI flag /GR is not added to CMAKE_CXX_FLAGS by default.
  cmake_policy(SET CMP0117 NEW)
endif()

project("Bitcoin Core"
  VERSION 24.99.0
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
set(COPYRIGHT_YEAR "2023")
set(COPYRIGHT_HOLDERS "The %s developers")
set(COPYRIGHT_HOLDERS_SUBSTITUTION ${PROJECT_NAME})
set(COPYRIGHT_HOLDERS_FINAL "The Bitcoin Core developers")

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

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
