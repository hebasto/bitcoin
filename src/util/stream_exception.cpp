// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <util/stream_exception.h>

#include <ios>
#include <string>

void ThrowStreamException(const std::string& message)
{
    throw std::ios_base::failure(message);
}
