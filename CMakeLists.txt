# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Going to be compatible with Qt 6.2 LTS.
# See https://doc.qt.io/qt-6.2/cmake-get-started.html
cmake_minimum_required(VERSION 3.16)

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
  link_libraries(-Wl,-O2)
endif()
