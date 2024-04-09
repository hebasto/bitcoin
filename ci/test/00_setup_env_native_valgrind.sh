#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"
export CONTAINER_NAME=ci_native_valgrind
export PACKAGES="valgrind clang-16 llvm-16 libclang-rt-16-dev python3-zmq libevent-dev libboost-dev libdb5.3++-dev libminiupnpc-dev libnatpmp-dev libzmq3-dev libsqlite3-dev"
export USE_VALGRIND=1
export NO_DEPENDS=1
export TEST_RUNNER_EXTRA="--exclude rpc_bind,feature_bind_extra"  # Excluded for now, see https://github.com/bitcoin/bitcoin/issues/17765#issuecomment-602068547
export GOAL="install"
export BITCOIN_CONFIG="-DWITH_ZMQ=ON -DWARN_INCOMPATIBLE_BDB=OFF -DBUILD_GUI=OFF -DCMAKE_C_COMPILER=clang-16 -DCMAKE_CXX_COMPILER=clang++-16" # TODO enable GUI
