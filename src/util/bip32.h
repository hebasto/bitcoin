// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <util/expected.h>

util::Expected<int, int> ParseKeyPathElement();

#endif // BITCOIN_UTIL_BIP32_H
