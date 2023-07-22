// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/univalue_helpers.h>

#include <univalue.h>

#include <optional>
#include <string>

namespace util {

std::optional<std::string> UniValueToString(const UniValue& uv)
{
    if (uv.isNull()) return {};
    return uv.get_str();
}

} // namespace util
