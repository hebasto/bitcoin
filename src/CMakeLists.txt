# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(AddWindowsResources)

configure_file(${PROJECT_SOURCE_DIR}/cmake/bitcoin-build-config.h.in bitcoin-build-config.h USE_SOURCE_PERMISSIONS @ONLY)
target_include_directories(core_interface INTERFACE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if (ENABLE_IPC AND NOT WITH_EXTERNAL_LIBMULTIPROCESS)
  include(../cmake/libmultiprocess.cmake)
  add_libmultiprocess(ipc/libmultiprocess)
endif()
if(ENABLE_IPC)
  add_subdirectory(ipc)
else()
  add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL ipc/stub.cpp)
  target_link_libraries(bitcoin_ipc PRIVATE core_interface)
endif()
