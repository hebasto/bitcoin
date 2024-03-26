#!/bin/sh
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

if [ -n "$SOURCE_DATE_EPOCH" ]; then
  find . -exec touch --date="@$SOURCE_DATE_EPOCH" {} +
fi

find . | sort | "$1" -X@ "$2"
