# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

set(abs_top_srcdir ${PROJECT_SOURCE_DIR})
set(abs_top_builddir ${PROJECT_BINARY_DIR})
set(EXEEXT ${CMAKE_EXECUTABLE_SUFFIX})

unset(ENABLE_WALLET_TRUE)
if(NOT ENABLE_WALLET)
  set(ENABLE_WALLET_TRUE \#)
endif()

unset(USE_SQLITE_TRUE)
if(NOT ENABLE_WALLET OR NOT WITH_SQLITE)
  set(USE_SQLITE_TRUE \#)
endif()

unset(USE_BDB_TRUE)
if(NOT ENABLE_WALLET OR NOT WITH_BDB)
  set(USE_BDB_TRUE \#)
endif()

unset(BUILD_BITCOIN_CLI_TRUE)
if(NOT BUILD_CLI)
  set(BUILD_BITCOIN_CLI_TRUE \#)
endif()

unset(BUILD_BITCOIN_UTIL_TRUE)
if(NOT BUILD_UTIL)
  set(BUILD_BITCOIN_UTIL_TRUE \#)
endif()

unset(BUILD_BITCOIN_WALLET_TRUE)
if(NOT BUILD_WALLET_TOOL)
  set(BUILD_BITCOIN_WALLET_TRUE \#)
endif()

unset(BUILD_BITCOIND_TRUE)
if(NOT BUILD_DAEMON)
  set(BUILD_BITCOIND_TRUE \#)
endif()

unset(ENABLE_ZMQ_TRUE)
if(NOT WITH_ZMQ)
  set(ENABLE_ZMQ_TRUE \#)
endif()

unset(ENABLE_EXTERNAL_SIGNER_TRUE)
if(NOT ENABLE_EXTERNAL_SIGNER)
  set(ENABLE_EXTERNAL_SIGNER_TRUE \#)
endif()

unset(ENABLE_SYSCALL_SANDBOX_TRUE)
if(NOT USE_SYSCALL_SANDBOX)
  set(ENABLE_SYSCALL_SANDBOX_TRUE \#)
endif()

unset(ENABLE_USDT_TRACEPOINTS_TRUE)
if(NOT ENABLE_TRACING)
  set(ENABLE_USDT_TRACEPOINTS_TRUE \#)
endif()

configure_file(config.ini.in config.ini @ONLY)

unset(abs_top_srcdir)
unset(abs_top_builddir)
unset(EXEEXT)
unset(ENABLE_WALLET_TRUE)
unset(USE_SQLITE_TRUE)
unset(USE_BDB_TRUE)
unset(BUILD_BITCOIN_CLI_TRUE)
unset(BUILD_BITCOIN_UTIL_TRUE)
unset(BUILD_BITCOIN_WALLET_TRUE)
unset(BUILD_BITCOIND_TRUE)
unset(ENABLE_ZMQ_TRUE)
unset(ENABLE_EXTERNAL_SIGNER_TRUE)
unset(ENABLE_SYSCALL_SANDBOX_TRUE)
unset(ENABLE_USDT_TRACEPOINTS_TRUE)


file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/functional)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fuzz)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/util)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
  file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/functional/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/functional/test_runner.py
    COPY_ON_ERROR SYMBOLIC
  )
  file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/fuzz/test_runner.py
    COPY_ON_ERROR SYMBOLIC
  )
  file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/util/rpcauth-test.py ${CMAKE_CURRENT_BINARY_DIR}/util/rpcauth-test.py
    COPY_ON_ERROR SYMBOLIC
  )
  file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/util/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/util/test_runner.py
    COPY_ON_ERROR SYMBOLIC
  )
else()
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/functional/test_runner.py)
  execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/functional/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/functional/test_runner.py)
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/fuzz/test_runner.py)
  execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/fuzz/test_runner.py)
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/util/rpcauth-test.py)
  execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/util/rpcauth-test.py ${CMAKE_CURRENT_BINARY_DIR}/util/rpcauth-test.py)
  file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/util/test_runner.py)
  execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/util/test_runner.py ${CMAKE_CURRENT_BINARY_DIR}/util/test_runner.py)
endif()
