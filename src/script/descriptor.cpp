// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/descriptor.h>

#include <util/bip32.h>
#include <util/expected.h>

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {

void ParseKeyPath(const std::vector<std::span<const char>>& split)
{
    auto parse_elem = [&](std::span<const char> elem) -> std::optional<uint32_t> {
        const auto parsed{ParseKeyPathElement(elem)};
        return parsed->ChildNumber();
    };

    const std::span<const char>& elem = split[0];
    const auto& op_num = parse_elem(elem);
}

} // namespace
