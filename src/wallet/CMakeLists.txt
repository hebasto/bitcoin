# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

if(NOT USE_SQLITE AND NOT USE_BDB)
  message(FATAL_ERROR "Wallet functionality requested but no BDB or SQLite support available.")
endif()

# Wallet functionality used by bitcoind and bitcoin-wallet executables.
add_library(bitcoin_wallet STATIC EXCLUDE_FROM_ALL
  coincontrol.cpp
  coinselection.cpp
  context.cpp
  crypter.cpp
  db.cpp
  dump.cpp
  external_signer_scriptpubkeyman.cpp
  feebumper.cpp
  fees.cpp
  interfaces.cpp
  load.cpp
  receive.cpp
  rpc/addresses.cpp
  rpc/backup.cpp
  rpc/coins.cpp
  rpc/encrypt.cpp
  rpc/spend.cpp
  rpc/signmessage.cpp
  rpc/transactions.cpp
  rpc/util.cpp
  rpc/wallet.cpp
  scriptpubkeyman.cpp
  spend.cpp
  transaction.cpp
  wallet.cpp
  walletdb.cpp
  walletutil.cpp
)
target_link_libraries(bitcoin_wallet
  PRIVATE
    bitcoin_common
    univalue
    Boost::headers
    $<$<BOOL:${USE_SQLITE}>:bitcoin_wallet_sqlite>
  PUBLIC
    $<$<BOOL:${USE_BDB}>:bitcoin_wallet_bdb>
)

add_library(bitcoin_wallet_sqlite OBJECT EXCLUDE_FROM_ALL sqlite.cpp)
target_link_libraries(bitcoin_wallet_sqlite PRIVATE $<TARGET_NAME_IF_EXISTS:PkgConfig::sqlite>)

add_library(bitcoin_wallet_bdb OBJECT EXCLUDE_FROM_ALL bdb.cpp salvage.cpp)
target_link_libraries(bitcoin_wallet_bdb PUBLIC $<TARGET_NAME_IF_EXISTS:BerkeleyDB::BerkeleyDB>)
