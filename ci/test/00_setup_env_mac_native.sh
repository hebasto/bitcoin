#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=x86_64-apple-darwin
# Homebrew's python@3.12 is marked as externally managed (PEP 668).
# Sticking to macOS system python for simplicity.
export CI_PYTHON_COMMAND="python"
export PIP_PACKAGES="zmq"
export GOAL="install"
export BITCOIN_CONFIG="--with-gui --with-miniupnpc --with-natpmp --enable-reduce-exports"
export CI_OS_NAME="macos"
export NO_DEPENDS=1
export OSX_SDK=""
export CCACHE_MAXSIZE=400M
export RUN_FUZZ_TESTS=true
