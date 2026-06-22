// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_UTIL_STREAM_EXCEPTION_H
#define BITCOIN_UTIL_STREAM_EXCEPTION_H

#include <string>

/**
 * Throw std::ios_base::failure.
 *
 * Provided so that callers can raise a stream exception
 * without `#include <ios>`.
 */
[[noreturn]] void ThrowStreamException(const std::string& message);

#endif // BITCOIN_UTIL_STREAM_EXCEPTION_H
