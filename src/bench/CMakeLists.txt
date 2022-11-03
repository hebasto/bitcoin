# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(GenerateHeaders)
generate_header_from_raw(data/block413567.raw)

add_executable(bench_bitcoin "")
target_sources(bench_bitcoin
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/data/block413567.raw.h
    addrman.cpp
    base58.cpp
    bech32.cpp
    bench.cpp
    bench_bitcoin.cpp
    block_assemble.cpp
    ccoins_caching.cpp
    chacha20.cpp
    chacha_poly_aead.cpp
    checkblock.cpp
    checkqueue.cpp
    crypto_hash.cpp
    data.cpp
    descriptors.cpp
    duplicate_inputs.cpp
    examples.cpp
    gcs_filter.cpp
    hashpadding.cpp
    lockedpool.cpp
    logging.cpp
    mempool_eviction.cpp
    mempool_stress.cpp
    merkle_root.cpp
    nanobench.cpp
    peer_eviction.cpp
    poly1305.cpp
    prevector.cpp
    rollingbloom.cpp
    rpc_blockchain.cpp
    rpc_mempool.cpp
    strencodings.cpp
    util_time.cpp
    verify_script.cpp
)
target_include_directories(bench_bitcoin
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)

if(TARGET bitcoinconsensus_static)
  target_link_libraries(bench_bitcoin PRIVATE bitcoinconsensus_static)
elseif(TARGET bitcoinconsensus AND NOT WIN32)
  target_link_libraries(bench_bitcoin PRIVATE bitcoinconsensus)
endif()
target_link_libraries(bench_bitcoin
  PRIVATE
    test_util
    leveldb
    univalue
    Boost::boost
)

if(ENABLE_WALLET)
  target_sources(bench_bitcoin
    PRIVATE
      coin_selection.cpp
      wallet_balance.cpp
      wallet_loading.cpp
  )
  target_link_libraries(bench_bitcoin PRIVATE bitcoin_wallet)
endif()
