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
    FastRandomContext rnd;
    uint64_t a64 = rnd.rand64();
    uint32_t b32 = rnd.rand32();
    uint64_t c64 = rnd.rand64();
    uint32_t d32 = rnd.rand32();
    bench.minEpochIterations(10000000).run([&] {
        FeeRateCompare(
            FeeFrac(std::bit_cast<std::int64_t>(a64++), std::bit_cast<std::int32_t>(b32++)),
            FeeFrac(std::bit_cast<std::int64_t>(c64++), std::bit_cast<std::int32_t>(d32++)));
    });
}

BENCHMARK(FeefracMultipication, benchmark::PriorityLevel::HIGH);
