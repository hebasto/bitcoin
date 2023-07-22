// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_UNIVALUE_HELPERS_H
#define BITCOIN_UTIL_UNIVALUE_HELPERS_H

#include <optional>
#include <string>

class UniValue;

namespace util {

std::optional<std::string> UniValueToString(const UniValue& uv);

} // namespace util

#endif // BITCOIN_UTIL_UNIVALUE_HELPERS_H
