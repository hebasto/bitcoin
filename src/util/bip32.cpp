// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bip32.h>

#include <tinyformat.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>

util::Expected<int, std::string> ParseKeyPathElement()
{
    return 42;
}
