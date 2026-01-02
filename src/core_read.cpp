// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <primitives/block.h> // IWYU pragma: keep
#include <streams.h>
#include <util/strencodings.h>

#include <exception>
#include <span>
#include <vector>

bool DecodeHexBlockHeader(CBlockHeader& header, const std::string& hex_header)
{
    if (!IsHex(hex_header)) return false;

    const std::vector<unsigned char> header_data{ParseHex(hex_header)};
    DataStream ser_header{header_data};
    try {
        ser_header >> header;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}
