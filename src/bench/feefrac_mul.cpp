// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>
#include <util/feefrac.h>

#include <bit>
#include <cstdint>

static void FeefracMultipication(benchmark::Bench& bench)
{
    FastRandomContext rand(true);
    bench.run([&] {
        int64_t a64 = std::bit_cast<std::int64_t>(rand.rand64());
        int32_t b32 = std::bit_cast<std::int32_t>(rand.rand32());
        int64_t c64 = std::bit_cast<std::int64_t>(rand.rand64());
        int32_t d32 = std::bit_cast<std::int32_t>(rand.rand32());
        FeeRateCompare(FeeFrac(a64, b32), FeeFrac(c64, d32));
    });
}

BENCHMARK(FeefracMultipication, benchmark::PriorityLevel::HIGH);
