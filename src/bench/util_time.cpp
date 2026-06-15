// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <test/util/time.h>
#include <util/time.h>

// https://en.cppreference.com/cpp/chrono/operator%22%22s
// > std::string also defines operator""s, to represent literal objects of type std::string, but it is a string literal: 10s is ten seconds, but "10"s is a two-character string.
#include <string>            // for operator""s

static void BenchTimeDeprecated(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)GetTime();
    });
}

static void BenchTimeMock(benchmark::Bench& bench)
{
    FakeNodeClock clock{111s};
    bench.run([&] {
        (void)GetTime<std::chrono::seconds>();
    });
}

static void BenchTimeMillis(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)GetTime<std::chrono::milliseconds>();
    });
}

static void BenchTimeMillisSys(benchmark::Bench& bench)
{
    bench.run([&] {
        (void)TicksSinceEpoch<std::chrono::milliseconds>(SystemClock::now());
    });
}

BENCHMARK(BenchTimeDeprecated);
BENCHMARK(BenchTimeMillis);
BENCHMARK(BenchTimeMillisSys);
BENCHMARK(BenchTimeMock);
