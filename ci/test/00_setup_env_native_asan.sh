#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CI_IMAGE_NAME_TAG="docker.io/ubuntu:24.04"

# Only install BCC tracing packages in CI. Container has to match the host for BCC to work.
if [[ "${INSTALL_BCC_TRACING_TOOLS}" == "true" ]]; then
  # Required for USDT functional tests to run
  BPFCC_PACKAGE="bpfcc-tools linux-headers-$(uname --kernel-release)"
  export CI_CONTAINER_CAP="--privileged -v /sys/kernel:/sys/kernel:rw"
else
  BPFCC_PACKAGE=""
  export CI_CONTAINER_CAP="--cap-add SYS_PTRACE"  # If run with (ASan + LSan), the container needs access to ptrace (https://github.com/google/sanitizers/issues/764)
fi

export CONTAINER_NAME=ci_native_asan
export PACKAGES="systemtap-sdt-dev clang-18 llvm-18 libclang-rt-18-dev python3-zmq libevent-dev libboost-dev ${BPFCC_PACKAGE}"
export NO_DEPENDS=1
export GOAL="install"
export BITCOIN_CONFIG="-DWITH_USDT=OFF -DWITH_ZMQ=OFF -DENABLE_WALLET=OFF -DBUILD_GUI=OFF \
-DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DBUILD_FUZZ_BINARY=OFF \
-DSANITIZERS=address,float-divide-by-zero,integer,undefined \
-DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 \
-DCMAKE_C_FLAGS='-ftrivial-auto-var-init=pattern' \
-DCMAKE_CXX_FLAGS='-DARENA_DEBUG -DDEBUG_LOCKORDER -ftrivial-auto-var-init=pattern'"
export CCACHE_MAXSIZE=300M

export RUN_UNIT_TESTS=false
export TEST_RUNNER_EXTRA="--filter feature_reindex.py"
