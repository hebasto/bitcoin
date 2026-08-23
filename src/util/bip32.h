// src/util/bip32.h

#ifndef BITCOIN_UTIL_BIP32_H
#define BITCOIN_UTIL_BIP32_H

#include <util/expected.h>

util::Expected<int, int> ParseKeyPathElement();

#endif // BITCOIN_UTIL_BIP32_H
