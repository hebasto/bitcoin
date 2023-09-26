# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

add_library(test_util STATIC EXCLUDE_FROM_ALL
  blockfilter.cpp
  coins.cpp
  index.cpp
  json.cpp
  logging.cpp
  mining.cpp
  net.cpp
  random.cpp
  script.cpp
  setup_common.cpp
  str.cpp
  transaction_utils.cpp
  txmempool.cpp
  validation.cpp
)
target_link_libraries(test_util
  PRIVATE
    core
    bitcoin_node
    bitcoin_common
    univalue
    Boost::headers
)

if(ENABLE_WALLET)
  target_sources(test_util
    PRIVATE
      ../../wallet/test/util.cpp
  )
endif()

target_link_libraries(test_util PRIVATE core)
