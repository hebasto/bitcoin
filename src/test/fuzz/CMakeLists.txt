# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

add_subdirectory(util)

add_executable(fuzz
  addition_overflow.cpp
)
target_link_libraries(fuzz
  core_interface
  test_fuzz
  bitcoin_cli
  bitcoin_common
  minisketch
  leveldb
  univalue
  secp256k1
  Boost::headers
  $<TARGET_NAME_IF_EXISTS:libevent::libevent>
)
