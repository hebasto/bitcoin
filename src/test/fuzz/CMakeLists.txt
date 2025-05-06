# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_subdirectory(util)

add_executable(fuzz)
target_link_libraries(fuzz
  core_interface
  fuzzer_interface
  test_fuzz
  bitcoin_cli
  bitcoin_common
  bitcoin_util
  minisketch
  leveldb
  univalue
  secp256k1
  Boost::headers
  libevent::extra
)

if(ENABLE_WALLET)
  add_subdirectory(${PROJECT_SOURCE_DIR}/src/wallet/test/fuzz wallet)
endif()
