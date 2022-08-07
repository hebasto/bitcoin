# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Optimized C library for ECDSA signatures and secret/public key operations on curve secp256k1.
# See https://github.com/bitcoin-core/secp256k1
add_library(secp256k1 STATIC EXCLUDE_FROM_ALL "")

target_sources(secp256k1
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src/secp256k1/src/secp256k1.c
    ${CMAKE_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult.c
    ${CMAKE_SOURCE_DIR}/src/secp256k1/src/precomputed_ecmult_gen.c
)

target_compile_definitions(secp256k1
  PRIVATE
    ECMULT_GEN_PREC_BITS=4
    ECMULT_WINDOW_SIZE=15
    ENABLE_MODULE_RECOVERY
    ENABLE_MODULE_SCHNORRSIG
    ENABLE_MODULE_EXTRAKEYS
)

target_include_directories(secp256k1
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/secp256k1/include>
)
