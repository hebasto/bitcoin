# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if(TARGET bitcoin-util AND TARGET bitcoin-tx AND TARGET Python3::Interpreter)
  add_test(NAME util_test_runner
    COMMAND Python3::Interpreter ${PROJECT_BINARY_DIR}/test/util/test_runner.py
  )
  set_property(TEST util_test_runner PROPERTY
    ENVIRONMENT_MODIFICATION
      BITCOINUTIL=set:$<TARGET_FILE:bitcoin-util>
      BITCOINTX=set:$<TARGET_FILE:bitcoin-tx>
  )
endif()

if(TARGET Python3::Interpreter)
  add_test(NAME util_rpcauth_test
    COMMAND Python3::Interpreter ${PROJECT_BINARY_DIR}/test/util/rpcauth-test.py
  )
endif()
