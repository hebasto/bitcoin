# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

include(CopyTargetForTests)

set(CRC32C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(CRC32C_USE_GLOG OFF CACHE BOOL "" FORCE)
set(CRC32C_INSTALL OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
mark_as_advanced(
  CRC32C_BUILD_TESTS
  CRC32C_BUILD_BENCHMARKS
  CRC32C_USE_GLOG
  CRC32C_INSTALL
)
add_subdirectory(crc32c EXCLUDE_FROM_ALL)

set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(HAVE_CRC32C ON CACHE BOOL "" FORCE)
set(HAVE_SNAPPY OFF CACHE BOOL "" FORCE)
set(HAVE_TCMALLOC OFF CACHE BOOL "" FORCE)
# Disabling TSA warnings because leveldb simultaneously
# enables -Werror as a usage requirement.
set(HAVE_CLANG_THREAD_SAFETY OFF CACHE BOOL "" FORCE)
mark_as_advanced(
  LEVELDB_BUILD_TESTS
  LEVELDB_BUILD_BENCHMARKS
  LEVELDB_INSTALL
  HAVE_CRC32C
  HAVE_SNAPPY
  HAVE_TCMALLOC
  HAVE_CLANG_THREAD_SAFETY
)
add_subdirectory(leveldb EXCLUDE_FROM_ALL)
target_compile_definitions(leveldb
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_WARNINGS>
)
target_compile_options(leveldb
  PRIVATE
    # RTTI has been forcibly disabled in CMake configuration since leveldb v1.23.
    # Commit: 69061b464ab1da287da9b7ffec1ed911b754403b
    # Using leveldb::Logger as a base class to derive CBitcoinLevelDBLogger
    # requires RTTI been enabled.
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-frtti>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/wd4722>
)
set_property(TARGET leveldb leveldbutil
  PROPERTY EXPORT_COMPILE_COMMANDS OFF
)

configure_file(${CMAKE_SOURCE_DIR}/cmake/bitcoin-config.h.in config/bitcoin-config.h @ONLY)
add_definitions(-DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(crypto)
add_subdirectory(univalue)
add_subdirectory(util)


# Stable, backwards-compatible consensus functionality
# also exposed as a shared library and/or a static one.
add_library(bitcoin_consensus OBJECT EXCLUDE_FROM_ALL "")
target_sources(bitcoin_consensus
  PRIVATE
    arith_uint256.cpp
    consensus/merkle.cpp
    consensus/tx_check.cpp
    hash.cpp
    primitives/block.cpp
    primitives/transaction.cpp
    pubkey.cpp
    script/interpreter.cpp
    script/script.cpp
    script/script_error.cpp
    uint256.cpp
    util/strencodings.cpp
)
target_include_directories(bitcoin_consensus
  PRIVATE
    secp256k1/include
)

if(WITH_ZMQ)
  add_subdirectory(zmq EXCLUDE_FROM_ALL)
endif()

# Home for common functionality shared by different executables and libraries.
# Similar to `bitcoin_util` library, but higher-level.
add_library(bitcoin_common STATIC EXCLUDE_FROM_ALL "")
target_sources(bitcoin_common
  PRIVATE
    $<TARGET_OBJECTS:bitcoin_consensus>
    base58.cpp
    bech32.cpp
    chainparams.cpp
    coins.cpp
    common/bloom.cpp
    common/run_command.cpp
    compressor.cpp
    core_read.cpp
    core_write.cpp
    deploymentinfo.cpp
    external_signer.cpp
    init/common.cpp
    key.cpp
    key_io.cpp
    merkleblock.cpp
    net_types.cpp
    netaddress.cpp
    netbase.cpp
    net_permissions.cpp
    outputtype.cpp
    policy/feerate.cpp
    policy/policy.cpp
    protocol.cpp
    psbt.cpp
    rpc/rawtransaction_util.cpp
    rpc/external_signer.cpp
    rpc/util.cpp
    scheduler.cpp
    script/descriptor.cpp
    script/miniscript.cpp
    script/sign.cpp
    script/signingprovider.cpp
    script/standard.cpp
    warnings.cpp
)
target_link_libraries(bitcoin_common
  PRIVATE
    bitcoin_util
    univalue
    secp256k1
    Boost::boost
)
if(TARGET PkgConfig::libevent)
  target_sources(bitcoin_common PRIVATE common/url.cpp)
  target_link_libraries(bitcoin_common PRIVATE PkgConfig::libevent)
endif()


set(installable_targets "")
if(ENABLE_WALLET)
  add_subdirectory(wallet)

  if(BUILD_WALLET_TOOL)
    add_executable(bitcoin-wallet "")
    target_sources(bitcoin-wallet
      PRIVATE
        bitcoin-wallet.cpp
        init/bitcoin-wallet.cpp
        wallet/wallettool.cpp
    )
    target_link_libraries(bitcoin-wallet
      PRIVATE
        bitcoin_wallet
        bitcoin_common
        bitcoin_util
        Boost::boost
    )
    copy_target_for_tests(bitcoin-wallet)
    list(APPEND installable_targets bitcoin-wallet)
  endif()
endif()


# P2P and RPC server functionality used by `bitcoind` and `bitcoin-qt` executables.
add_library(bitcoin_node STATIC EXCLUDE_FROM_ALL "")
target_sources(bitcoin_node
  PRIVATE
    addrdb.cpp
    addrman.cpp
    banman.cpp
    blockencodings.cpp
    blockfilter.cpp
    chain.cpp
    consensus/tx_verify.cpp
    dbwrapper.cpp
    deploymentstatus.cpp
    flatfile.cpp
    headerssync.cpp
    httprpc.cpp
    httpserver.cpp
    i2p.cpp
    index/base.cpp
    index/blockfilterindex.cpp
    index/coinstatsindex.cpp
    index/txindex.cpp
    init.cpp
    kernel/chain.cpp
    kernel/checks.cpp
    kernel/coinstats.cpp
    kernel/context.cpp
    kernel/mempool_persist.cpp
    mapport.cpp
    net.cpp
    netgroup.cpp
    net_processing.cpp
    node/blockstorage.cpp
    node/caches.cpp
    node/chainstate.cpp
    node/chainstatemanager_args.cpp
    node/coin.cpp
    node/connection_types.cpp
    node/context.cpp
    node/eviction.cpp
    node/interface_ui.cpp
    node/interfaces.cpp
    node/mempool_args.cpp
    node/mempool_persist_args.cpp
    node/miner.cpp
    node/minisketchwrapper.cpp
    node/psbt.cpp
    node/transaction.cpp
    node/txreconciliation.cpp
    node/utxo_snapshot.cpp
    node/validation_cache_args.cpp
    noui.cpp
    policy/fees.cpp
    policy/fees_args.cpp
    policy/packages.cpp
    policy/rbf.cpp
    policy/settings.cpp
    pow.cpp
    rest.cpp
    rpc/blockchain.cpp
    rpc/fees.cpp
    rpc/mempool.cpp
    rpc/mining.cpp
    rpc/net.cpp
    rpc/node.cpp
    rpc/output_script.cpp
    rpc/rawtransaction.cpp
    rpc/server.cpp
    rpc/server_util.cpp
    rpc/signmessage.cpp
    rpc/txoutproof.cpp
    script/sigcache.cpp
    shutdown.cpp
    signet.cpp
    timedata.cpp
    torcontrol.cpp
    txdb.cpp
    txmempool.cpp
    txorphanage.cpp
    txrequest.cpp
    validation.cpp
    validationinterface.cpp
    versionbits.cpp
)
if(ENABLE_WALLET)
  target_sources(bitcoin_node PRIVATE wallet/init.cpp)
  target_link_libraries(bitcoin_node PRIVATE bitcoin_wallet)
  if(USE_BDB)
    target_link_libraries(bitcoin_node PRIVATE BerkeleyDB::BerkeleyDB)
  endif()
else()
  target_sources(bitcoin_node PRIVATE dummywallet.cpp)
endif()
target_link_libraries(bitcoin_node
  PRIVATE
    bitcoin_util
    bitcoin_common
    minisketch
    leveldb
    univalue
    Boost::boost
    $<$<NOT:$<PLATFORM_ID:Windows>>:PkgConfig::libevent_pthreads>
)
if(TARGET NATPMP::NATPMP)
  target_link_libraries(bitcoin_node PRIVATE NATPMP::NATPMP)
endif()
if(TARGET MiniUPnPc::MiniUPnPc)
  target_link_libraries(bitcoin_node PRIVATE MiniUPnPc::MiniUPnPc)
endif()
if(TARGET bitcoin_zmq)
  target_link_libraries(bitcoin_node PRIVATE bitcoin_zmq)
endif()


# Bitcoin Core bitcoind
if(BUILD_DAEMON)
  add_executable(bitcoind "")
  target_sources(bitcoind
    PRIVATE
      bitcoind.cpp
      init/bitcoind.cpp
  )
  target_link_libraries(bitcoind
    PRIVATE
      bitcoin_node
      $<$<BOOL:${ENABLE_WALLET}>:bitcoin_wallet>
  )
  copy_target_for_tests(bitcoind)
  list(APPEND installable_targets bitcoind)
endif()


if(BUILD_CLI OR NOT WITH_GUI STREQUAL "no")
  add_library(rpc_client STATIC EXCLUDE_FROM_ALL "")
  target_sources(rpc_client PRIVATE rpc/client.cpp)
  target_link_libraries(rpc_client PUBLIC univalue)
endif()


# Bitcoin Core RPC client
if(BUILD_CLI)
  add_executable(bitcoin-cli "")
  target_sources(bitcoin-cli
    PRIVATE
      bitcoin-cli.cpp
      compat/stdin.cpp
  )
  target_link_libraries(bitcoin-cli
    PRIVATE
      bitcoin_common
      bitcoin_util
      rpc_client
      PkgConfig::libevent
  )
  copy_target_for_tests(bitcoin-cli)
  list(APPEND installable_targets bitcoin-cli)
endif()


if(BUILD_TX)
  add_executable(bitcoin-tx "")
  target_sources(bitcoin-tx
    PRIVATE
      bitcoin-tx.cpp
  )
  target_link_libraries(bitcoin-tx
    PRIVATE
      bitcoin_common
      bitcoin_util
      univalue
  )
  copy_target_for_tests(bitcoin-tx)
  list(APPEND installable_targets bitcoin-tx)
endif()


if(BUILD_UTIL)
  add_executable(bitcoin-util "")
  target_sources(bitcoin-util
    PRIVATE
      bitcoin-util.cpp
  )
  target_link_libraries(bitcoin-util
    PRIVATE
      bitcoin_common
      bitcoin_util
  )
  copy_target_for_tests(bitcoin-util)
  list(APPEND installable_targets bitcoin-util)
endif()


if(BUILD_BITCOINCONSENSUS_LIB)
  add_subdirectory(script)
endif()

if(BUILD_BITCOINKERNEL_LIB)
  add_subdirectory(kernel)
endif()

if(BUILD_UTIL_CHAINSTATE)
  add_executable(bitcoin-chainstate "")
  target_sources(bitcoin-chainstate
    PRIVATE
      $<TARGET_OBJECTS:bitcoin_crypto>
      bitcoin-chainstate.cpp
  )
  if(TARGET bitcoinkernel_static)
    target_link_libraries(bitcoin-chainstate PRIVATE bitcoinkernel_static)
  elseif(TARGET bitcoinkernel AND NOT WIN32)
    target_link_libraries(bitcoin-chainstate PRIVATE bitcoinkernel)
  endif()
  target_link_libraries(bitcoin-chainstate
    PRIVATE
      leveldb
      univalue
      secp256k1
      Boost::boost
      $<$<PLATFORM_ID:Windows>:ws2_32>
  )
  if(TARGET std_filesystem)
    target_link_libraries(bitcoin-chainstate PRIVATE std_filesystem)
  endif()
  list(APPEND installable_targets bitcoin-chainstate)
endif()


if(NOT WITH_GUI STREQUAL "no")
  add_subdirectory(qt)
endif()

add_subdirectory(test/util)
if(BUILD_TESTS)
  add_subdirectory(test)
endif()
if(BUILD_BENCH)
  add_subdirectory(bench)
endif()


include(GNUInstallDirs)
install(TARGETS ${installable_targets}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(INSTALL_MAN)
  install(DIRECTORY ../doc/man/
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
    FILES_MATCHING PATTERN *.1
  )
endif()
