# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_library(bitcoin_ipc STATIC EXCLUDE_FROM_ALL
  capnp/mining.cpp
  capnp/protocol.cpp
  interfaces.cpp
  process.cpp
)

target_capnp_sources(bitcoin_ipc ${PROJECT_SOURCE_DIR}
  capnp/common.capnp
  capnp/echo.capnp
  capnp/init.capnp
  capnp/mining.capnp
)

target_link_libraries(bitcoin_ipc
  PRIVATE
    core_interface
    univalue
)

if(BUILD_TESTS)
  add_library(bitcoin_ipc_test STATIC EXCLUDE_FROM_ALL
    test/ipc_test.cpp
  )
  target_capnp_sources(bitcoin_ipc_test ${PROJECT_SOURCE_DIR}
    test/ipc_test.capnp
  )
  add_dependencies(bitcoin_ipc_test bitcoin_ipc_headers)

  target_link_libraries(bitcoin_ipc_test
    PRIVATE
      core_interface
      univalue
      Boost::headers
  )
endif()

configure_file(.clang-tidy.in .clang-tidy USE_SOURCE_PERMISSIONS COPYONLY)
