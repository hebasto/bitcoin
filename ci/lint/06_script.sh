#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C

echo "CIRRUS_BRANCH = $CIRRUS_BRANCH"
echo "CIRRUS_BASE_BRANCH = $CIRRUS_BASE_BRANCH"

test/lint/lint-all.sh

if [ "$CIRRUS_REPO_FULL_NAME" = "bitcoin/bitcoin" ] && [ -n "$CIRRUS_CRON" ]; then
    git log --merges --before="2 days ago" -1 --format='%H' > ./contrib/verify-commits/trusted-sha512-root-commit
    ${CI_RETRY_EXE}  gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys $(<contrib/verify-commits/trusted-keys) &&
    ./contrib/verify-commits/verify-commits.py --clean-merge=2;
fi
