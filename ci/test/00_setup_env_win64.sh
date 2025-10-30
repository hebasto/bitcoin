#!/usr/bin/env bash
#
# Copyright (c) 2019-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_win64
export CI_IMAGE_NAME_TAG="mirror.gcr.io/fedora:42"  # Check that https://fedora.pkgs.org/42/fedora-x86_64/ucrt64-gcc-c++-14.2.1-4.fc42.x86_64.rpm.html can cross-compile
export CI_BASE_PACKAGES="cmake gawk make patch procps-ng rsync ucrt64-gcc-c++ util-linux which"
export HOST=x86_64-w64-mingw32ucrt
export DEP_OPTS="CC=/usr/bin/${HOST}-gcc CXX=/usr/bin/${HOST}-g++ NO_QT=1"
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="deploy"
export BITCOIN_CONFIG="-DREDUCE_EXPORTS=ON -DBUILD_GUI_TESTS=OFF \
-DCMAKE_CXX_FLAGS='-Wno-error=maybe-uninitialized'"
