#!/usr/bin/env bash
#
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_openbsd_cross
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export APT_LLVM_V="22"
export OPENBSD_VERSION=7.9
export PACKAGES="clang-${APT_LLVM_V} llvm-${APT_LLVM_V} lld"
export HOST=x86_64-unknown-openbsd
export DEP_OPTS="build_CC=clang build_CXX=clang++ AR=llvm-ar-${APT_LLVM_V} STRIP=llvm-strip-${APT_LLVM_V} NM=llvm-nm-${APT_LLVM_V} RANLIB=llvm-ranlib-${APT_LLVM_V} NO_QT=1"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DREDUCE_EXPORTS=ON \
 -DBUILD_GUI=OFF \
 -DWITH_USDT=OFF \
"
export RUN_UNIT_TESTS="false"
export RUN_FUNCTIONAL_TESTS="false"
