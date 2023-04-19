#!/usr/bin/env bash
#
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tsan
export DOCKER_NAME_TAG=ubuntu:22.04
export PACKAGES="clang-12 llvm-12 libc++abi-12-dev libc++-12-dev python3-zmq libunwind-dev"
export DEP_OPTS="NO_QT=1 NO_ZMQ=1 CC=clang-12 CXX='clang++-12 -stdlib=libc++'"
export GOAL="install"
export BITCOIN_CONFIG="--disable-fuzz-binary --disable-tests --disable-bench CPPFLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER' CXXFLAGS='-g' --with-sanitizers=thread"
export RUN_UNIT_TESTS=false
