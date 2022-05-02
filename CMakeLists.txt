# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# We are able to build on Ubuntu Bionic.
# On the other hand, some useful features lack:
#  - 3.12: Object libraries can be linked to with target_link_libraries()
#  - 3.12: If the CONFIGURE_DEPENDS flag is specified, CMake will add logic to the main
#          build system check target to rerun the flagged GLOB commands at build time
#  - ...
cmake_minimum_required(VERSION 3.10)

project("Bitcoin Core"
  VERSION 23.0.0
  DESCRIPTION "Bitcoin client software"
  HOMEPAGE_URL "https://bitcoincore.org/"
  LANGUAGES CXX C ASM
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(-pipe -O2)
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL arm)
    add_compile_options(-Wno-psabi)
  endif()
  link_libraries(-Wl,-O2)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  add_compile_definitions(MAC_OSX OBJC_OLD_DISPATCH_PROTOTYPES=0)
  link_libraries(-Wl,-headerpad_max_install_names)
endif()
