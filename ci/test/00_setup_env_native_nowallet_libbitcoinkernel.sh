#!/usr/bin/env bash
#
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_nowallet_libbitcoinkernel
export CI_IMAGE_NAME_TAG=debian:buster
# Use minimum supported python3.7 and clang-8, see doc/dependencies.md
export PACKAGES="-t buster-backports python3-zmq clang-8 llvm-8 libc++abi-8-dev libc++-8-dev"
export APPEND_APT_SOURCES_LIST="deb http://deb.debian.org/debian buster-backports main"
export DEP_OPTS="NO_WALLET=1 NO_QT=1 CC=clang-8 CXX='clang++-8 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DBUILD_UTIL_CHAINSTATE=ON -DBUILD_BITCOINKERNEL_LIB=ON"
