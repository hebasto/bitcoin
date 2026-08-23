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

struct KeyPathElement {
    /** Derivation index, without the hardened flag */
    uint32_t index;
    bool is_hardened;
};

/** Parse a single key path element like "0", "0'", or "0h".
 *  Returns the derivation index and hardened status, or an error message. */
util::Expected<KeyPathElement, std::string> ParseKeyPathElement(std::span<const char> elem);

#endif // BITCOIN_UTIL_BIP32_H
