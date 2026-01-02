// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_IO_H
#define BITCOIN_CORE_IO_H

#include <consensus/amount.h>
#include <util/result.h>

#include <string>
#include <vector>
#include <optional>

class CBlock;
class CBlockHeader;
class CScript;
class CTransaction;
struct CMutableTransaction;
class SigningProvider;
class uint256;
class UniValue;
class CTxUndo;
class CTxOut;

void DecodeHexBlockHeader(CBlockHeader&, const std::string& hex_header);

#endif // BITCOIN_CORE_IO_H
