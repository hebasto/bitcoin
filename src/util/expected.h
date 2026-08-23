#ifndef BITCOIN_UTIL_EXPECTED_H
#define BITCOIN_UTIL_EXPECTED_H

#include <expected>

namespace util {
template <class T, class E>
using Expected = std::expected<T, E>;
template <class E>
using Unexpected = std::unexpected<E>;
} // namespace util

#endif // BITCOIN_UTIL_EXPECTED_H
