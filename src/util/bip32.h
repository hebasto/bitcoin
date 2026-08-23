// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <cstdint>
#include <span>
#include <string>
#include <util/expected.h>
#include <vector>

util::Expected<int, std::string> ParseKeyPathElement();

#endif // BITCOIN_UTIL_BIP32_H
