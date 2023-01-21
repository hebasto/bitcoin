// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/system.h>

#include <clientversion.h>
#include <fs.h>
#include <hash.h> // For Hash()
#include <key.h>  // For CKey
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/getuniquepath.h>
#include <util/message.h> // For MessageSign(), MessageVerify(), MESSAGE_MAGIC
#include <util/moneystr.h>
#include <util/overflow.h>
#include <util/readwritefile.h>
#include <util/spanparsing.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/vector.h>
#include <util/bitdeque.h>

#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <optional>
#include <stdint.h>
#include <string.h>
#include <thread>
#include <univalue.h>
#include <utility>
#include <vector>
#ifndef WIN32
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <boost/test/unit_test.hpp>

using namespace std::literals;
static const std::string STRING_WITH_EMBEDDED_NULL_CHAR{"1"s "\0" "1"s};

/* defined in logging.cpp */
namespace BCLog {
    std::string LogEscapeMessage(const std::string& str);
}

BOOST_FIXTURE_TEST_SUITE(util_tests, BasicTestingSetup)

namespace {
class NoCopyOrMove
{
public:
    int i;
    explicit NoCopyOrMove(int i) : i{i} { }

    NoCopyOrMove() = delete;
    NoCopyOrMove(const NoCopyOrMove&) = delete;
    NoCopyOrMove(NoCopyOrMove&&) = delete;
    NoCopyOrMove& operator=(const NoCopyOrMove&) = delete;
    NoCopyOrMove& operator=(NoCopyOrMove&&) = delete;

    operator bool() const { return i != 0; }

    int get_ip1() { return i + 1; }
    bool test()
    {
        // Check that Assume can be used within a lambda and still call methods
        [&]() { Assume(get_ip1()); }();
        return Assume(get_ip1() != 5);
    }
};
} // namespace

BOOST_AUTO_TEST_CASE(util_check)
{
    // Check that Assert can forward
    const std::unique_ptr<int> p_two = Assert(std::make_unique<int>(2));
    // Check that Assert works on lvalues and rvalues
    const int two = *Assert(p_two);
    Assert(two == 2);
    Assert(true);
    // Check that Assume can be used as unary expression
    const bool result{Assume(two == 2)};
    Assert(result);

    // Check that Assert doesn't require copy/move
    NoCopyOrMove x{9};
    Assert(x).i += 3;
    Assert(x).test();

    // Check nested Asserts
    BOOST_CHECK_EQUAL(Assert((Assert(x).test() ? 3 : 0)), 3);

    // Check -Wdangling-gsl does not trigger when copying the int. (It would
    // trigger on "const int&")
    const int nine{*Assert(std::optional<int>{9})};
    BOOST_CHECK_EQUAL(9, nine);
}

BOOST_AUTO_TEST_CASE(util_criticalsection)
{
    RecursiveMutex cs;

    do {
        LOCK(cs);
        break;

        BOOST_ERROR("break was swallowed!");
    } while(0);

    do {
        TRY_LOCK(cs, lockTest);
        if (lockTest) {
            BOOST_CHECK(true); // Needed to suppress "Test case [...] did not check any assertions"
            break;
        }

        BOOST_ERROR("break was swallowed!");
    } while(0);
}

static const unsigned char ParseHex_expected[65] = {
    0x04, 0x67, 0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19, 0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7,
    0x10, 0x5c, 0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79, 0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde,
    0xb6, 0x49, 0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3, 0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12,
    0xde, 0x5c, 0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a, 0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d,
    0x5f
};
BOOST_AUTO_TEST_CASE(span_write_bytes)
{
    std::array mut_arr{uint8_t{0xaa}, uint8_t{0xbb}};
    const auto mut_bytes{MakeWritableByteSpan(mut_arr)};
    mut_bytes[1] = std::byte{0x11};
    BOOST_CHECK_EQUAL(mut_arr.at(0), 0xaa);
    BOOST_CHECK_EQUAL(mut_arr.at(1), 0x11);
}

BOOST_AUTO_TEST_CASE(util_Join)
{
    // Normal version
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{}, ", "), "");
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{"foo"}, ", "), "foo");
    BOOST_CHECK_EQUAL(Join(std::vector<std::string>{"foo", "bar"}, ", "), "foo, bar");

    // Version with unary operator
    const auto op_upper = [](const std::string& s) { return ToUpper(s); };
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{}, ", ", op_upper), "");
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{"foo"}, ", ", op_upper), "FOO");
    BOOST_CHECK_EQUAL(Join(std::list<std::string>{"foo", "bar"}, ", ", op_upper), "FOO, BAR");
}

BOOST_AUTO_TEST_CASE(util_ReplaceAll)
{
    const std::string original("A test \"%s\" string '%s'.");
    auto test_replaceall = [&original](const std::string& search, const std::string& substitute, const std::string& expected) {
        auto test = original;
        ReplaceAll(test, search, substitute);
        BOOST_CHECK_EQUAL(test, expected);
    };

    test_replaceall("", "foo", original);
    test_replaceall(original, "foo", "foo");
    test_replaceall("%s", "foo", "A test \"foo\" string 'foo'.");
    test_replaceall("\"", "foo", "A test foo%sfoo string '%s'.");
    test_replaceall("'", "foo", "A test \"%s\" string foo%sfoo.");
}

BOOST_AUTO_TEST_CASE(util_FormatISO8601DateTime)
{
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(1317425777), "2011-09-30T23:36:17Z");
    BOOST_CHECK_EQUAL(FormatISO8601DateTime(0), "1970-01-01T00:00:00Z");
}

BOOST_AUTO_TEST_CASE(util_FormatMoney)
{
    BOOST_CHECK_EQUAL(FormatMoney(0), "0.00");
    BOOST_CHECK_EQUAL(FormatMoney((COIN/10000)*123456789), "12345.6789");
    BOOST_CHECK_EQUAL(FormatMoney(-COIN), "-1.00");

    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000000), "100000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000000), "10000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000000), "1000000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100000), "100000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10000), "10000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*1000), "1000.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*100), "100.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN*10), "10.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN), "1.00");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10), "0.10");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100), "0.01");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000), "0.001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000), "0.0001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000), "0.00001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/1000000), "0.000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/10000000), "0.0000001");
    BOOST_CHECK_EQUAL(FormatMoney(COIN/100000000), "0.00000001");

    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max()), "92233720368.54775807");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 1), "92233720368.54775806");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 2), "92233720368.54775805");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::max() - 3), "92233720368.54775804");
    // ...
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 3), "-92233720368.54775805");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 2), "-92233720368.54775806");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min() + 1), "-92233720368.54775807");
    BOOST_CHECK_EQUAL(FormatMoney(std::numeric_limits<CAmount>::min()), "-92233720368.54775808");
}

/* Test strprintf formatting directives.
 * Put a string before and after to ensure sanity of element sizes on stack. */
#define B "check_prefix"
#define E "check_postfix"
BOOST_AUTO_TEST_CASE(strprintf_numbers)
{
    int64_t s64t = -9223372036854775807LL; /* signed 64 bit test value */
    uint64_t u64t = 18446744073709551615ULL; /* unsigned 64 bit test value */
    BOOST_CHECK(strprintf("%s %d %s", B, s64t, E) == B" -9223372036854775807 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, u64t, E) == B" 18446744073709551615 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, u64t, E) == B" ffffffffffffffff " E);

    size_t st = 12345678; /* unsigned size_t test value */
    ssize_t sst = -12345678; /* signed size_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, sst, E) == B" -12345678 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, st, E) == B" 12345678 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, st, E) == B" bc614e " E);

    ptrdiff_t pt = 87654321; /* positive ptrdiff_t test value */
    ptrdiff_t spt = -87654321; /* negative ptrdiff_t test value */
    BOOST_CHECK(strprintf("%s %d %s", B, spt, E) == B" -87654321 " E);
    BOOST_CHECK(strprintf("%s %u %s", B, pt, E) == B" 87654321 " E);
    BOOST_CHECK(strprintf("%s %x %s", B, pt, E) == B" 5397fb1 " E);
}
#undef B
#undef E

BOOST_AUTO_TEST_CASE(test_IsDigit)
{
    BOOST_CHECK_EQUAL(IsDigit('0'), true);
    BOOST_CHECK_EQUAL(IsDigit('1'), true);
    BOOST_CHECK_EQUAL(IsDigit('8'), true);
    BOOST_CHECK_EQUAL(IsDigit('9'), true);

    BOOST_CHECK_EQUAL(IsDigit('0' - 1), false);
    BOOST_CHECK_EQUAL(IsDigit('9' + 1), false);
    BOOST_CHECK_EQUAL(IsDigit(0), false);
    BOOST_CHECK_EQUAL(IsDigit(1), false);
    BOOST_CHECK_EQUAL(IsDigit(8), false);
    BOOST_CHECK_EQUAL(IsDigit(9), false);
}

/* Check for overflow */
template <typename T>
static void TestAddMatrixOverflow()
{
    constexpr T MAXI{std::numeric_limits<T>::max()};
    BOOST_CHECK(!CheckedAdd(T{1}, MAXI));
    BOOST_CHECK(!CheckedAdd(MAXI, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{1}, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(MAXI, MAXI));

    BOOST_CHECK_EQUAL(0, CheckedAdd(T{0}, T{0}).value());
    BOOST_CHECK_EQUAL(MAXI, CheckedAdd(T{0}, MAXI).value());
    BOOST_CHECK_EQUAL(MAXI, CheckedAdd(T{1}, MAXI - 1).value());
    BOOST_CHECK_EQUAL(MAXI - 1, CheckedAdd(T{1}, MAXI - 2).value());
    BOOST_CHECK_EQUAL(0, SaturatingAdd(T{0}, T{0}));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{0}, MAXI));
    BOOST_CHECK_EQUAL(MAXI, SaturatingAdd(T{1}, MAXI - 1));
    BOOST_CHECK_EQUAL(MAXI - 1, SaturatingAdd(T{1}, MAXI - 2));
}

/* Check for overflow or underflow */
template <typename T>
static void TestAddMatrix()
{
    TestAddMatrixOverflow<T>();
    constexpr T MINI{std::numeric_limits<T>::min()};
    constexpr T MAXI{std::numeric_limits<T>::max()};
    BOOST_CHECK(!CheckedAdd(T{-1}, MINI));
    BOOST_CHECK(!CheckedAdd(MINI, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{-1}, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(MINI, MINI));

    BOOST_CHECK_EQUAL(MINI, CheckedAdd(T{0}, MINI).value());
    BOOST_CHECK_EQUAL(MINI, CheckedAdd(T{-1}, MINI + 1).value());
    BOOST_CHECK_EQUAL(-1, CheckedAdd(MINI, MAXI).value());
    BOOST_CHECK_EQUAL(MINI + 1, CheckedAdd(T{-1}, MINI + 2).value());
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{0}, MINI));
    BOOST_CHECK_EQUAL(MINI, SaturatingAdd(T{-1}, MINI + 1));
    BOOST_CHECK_EQUAL(MINI + 1, SaturatingAdd(T{-1}, MINI + 2));
    BOOST_CHECK_EQUAL(-1, SaturatingAdd(MINI, MAXI));
}

template <typename T>
static void RunToIntegralTests()
{
    BOOST_CHECK(!ToIntegral<T>(STRING_WITH_EMBEDDED_NULL_CHAR));
    BOOST_CHECK(!ToIntegral<T>(" 1"));
    BOOST_CHECK(!ToIntegral<T>("1 "));
    BOOST_CHECK(!ToIntegral<T>("1a"));
    BOOST_CHECK(!ToIntegral<T>("1.1"));
    BOOST_CHECK(!ToIntegral<T>("1.9"));
    BOOST_CHECK(!ToIntegral<T>("+01.9"));
    BOOST_CHECK(!ToIntegral<T>("-"));
    BOOST_CHECK(!ToIntegral<T>("+"));
    BOOST_CHECK(!ToIntegral<T>(" -1"));
    BOOST_CHECK(!ToIntegral<T>("-1 "));
    BOOST_CHECK(!ToIntegral<T>(" -1 "));
    BOOST_CHECK(!ToIntegral<T>("+1"));
    BOOST_CHECK(!ToIntegral<T>(" +1"));
    BOOST_CHECK(!ToIntegral<T>(" +1 "));
    BOOST_CHECK(!ToIntegral<T>("+-1"));
    BOOST_CHECK(!ToIntegral<T>("-+1"));
    BOOST_CHECK(!ToIntegral<T>("++1"));
    BOOST_CHECK(!ToIntegral<T>("--1"));
    BOOST_CHECK(!ToIntegral<T>(""));
    BOOST_CHECK(!ToIntegral<T>("aap"));
    BOOST_CHECK(!ToIntegral<T>("0x1"));
    BOOST_CHECK(!ToIntegral<T>("-32482348723847471234"));
    BOOST_CHECK(!ToIntegral<T>("32482348723847471234"));
}

BOOST_AUTO_TEST_CASE(test_ToIntegral)
{
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("1234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("0").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("01234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("00000000000000001234").value(), 1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-00000000000000001234").value(), -1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("00000000000000000000").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-00000000000000000000").value(), 0);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-1234").value(), -1'234);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-1").value(), -1);

    RunToIntegralTests<uint64_t>();
    RunToIntegralTests<int64_t>();
    RunToIntegralTests<uint32_t>();
    RunToIntegralTests<int32_t>();
    RunToIntegralTests<uint16_t>();
    RunToIntegralTests<int16_t>();
    RunToIntegralTests<uint8_t>();
    RunToIntegralTests<int8_t>();

    BOOST_CHECK(!ToIntegral<int64_t>("-9223372036854775809"));
    BOOST_CHECK_EQUAL(ToIntegral<int64_t>("-9223372036854775808").value(), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(ToIntegral<int64_t>("9223372036854775807").value(), 9'223'372'036'854'775'807);
    BOOST_CHECK(!ToIntegral<int64_t>("9223372036854775808"));

    BOOST_CHECK(!ToIntegral<uint64_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint64_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint64_t>("18446744073709551615").value(), 18'446'744'073'709'551'615ULL);
    BOOST_CHECK(!ToIntegral<uint64_t>("18446744073709551616"));

    BOOST_CHECK(!ToIntegral<int32_t>("-2147483649"));
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("-2147483648").value(), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(ToIntegral<int32_t>("2147483647").value(), 2'147'483'647);
    BOOST_CHECK(!ToIntegral<int32_t>("2147483648"));

    BOOST_CHECK(!ToIntegral<uint32_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint32_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint32_t>("4294967295").value(), 4'294'967'295U);
    BOOST_CHECK(!ToIntegral<uint32_t>("4294967296"));

    BOOST_CHECK(!ToIntegral<int16_t>("-32769"));
    BOOST_CHECK_EQUAL(ToIntegral<int16_t>("-32768").value(), -32'768);
    BOOST_CHECK_EQUAL(ToIntegral<int16_t>("32767").value(), 32'767);
    BOOST_CHECK(!ToIntegral<int16_t>("32768"));

    BOOST_CHECK(!ToIntegral<uint16_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint16_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint16_t>("65535").value(), 65'535U);
    BOOST_CHECK(!ToIntegral<uint16_t>("65536"));

    BOOST_CHECK(!ToIntegral<int8_t>("-129"));
    BOOST_CHECK_EQUAL(ToIntegral<int8_t>("-128").value(), -128);
    BOOST_CHECK_EQUAL(ToIntegral<int8_t>("127").value(), 127);
    BOOST_CHECK(!ToIntegral<int8_t>("128"));

    BOOST_CHECK(!ToIntegral<uint8_t>("-1"));
    BOOST_CHECK_EQUAL(ToIntegral<uint8_t>("0").value(), 0U);
    BOOST_CHECK_EQUAL(ToIntegral<uint8_t>("255").value(), 255U);
    BOOST_CHECK(!ToIntegral<uint8_t>("256"));
}

int64_t atoi64_legacy(const std::string& str)
{
    return strtoll(str.c_str(), nullptr, 10);
}

BOOST_AUTO_TEST_CASE(test_LocaleIndependentAtoi)
{
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1234"), 1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("0"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("01234"), 1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1234"), -1'234);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" 1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1 "), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1a"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1.1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("1.9"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+01.9"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1"), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" -1"), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-1 "), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" -1 "), -1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" +1"), 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(" +1 "), 1);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("+-1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-+1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("++1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("--1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>(""), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("aap"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("0x1"), 0);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-32482348723847471234"), -2'147'483'647 - 1);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("32482348723847471234"), 2'147'483'647);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("-9223372036854775809"), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("-9223372036854775808"), -9'223'372'036'854'775'807LL - 1LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("9223372036854775807"), 9'223'372'036'854'775'807);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>("9223372036854775808"), 9'223'372'036'854'775'807);

    std::map<std::string, int64_t> atoi64_test_pairs = {
        {"-9223372036854775809", std::numeric_limits<int64_t>::min()},
        {"-9223372036854775808", -9'223'372'036'854'775'807LL - 1LL},
        {"9223372036854775807", 9'223'372'036'854'775'807},
        {"9223372036854775808", std::numeric_limits<int64_t>::max()},
        {"+-", 0},
        {"0x1", 0},
        {"ox1", 0},
        {"", 0},
    };

    for (const auto& pair : atoi64_test_pairs) {
        BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>(pair.first), pair.second);
    }

    // Ensure legacy compatibility with previous versions of Bitcoin Core's atoi64
    for (const auto& pair : atoi64_test_pairs) {
        BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int64_t>(pair.first), atoi64_legacy(pair.first));
    }

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("18446744073709551615"), 18'446'744'073'709'551'615ULL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint64_t>("18446744073709551616"), 18'446'744'073'709'551'615ULL);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-2147483649"), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("-2147483648"), -2'147'483'648LL);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("2147483647"), 2'147'483'647);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int32_t>("2147483648"), 2'147'483'647);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("4294967295"), 4'294'967'295U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint32_t>("4294967296"), 4'294'967'295U);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("-32769"), -32'768);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("-32768"), -32'768);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("32767"), 32'767);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int16_t>("32768"), 32'767);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("65535"), 65'535U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint16_t>("65536"), 65'535U);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("-129"), -128);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("-128"), -128);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("127"), 127);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<int8_t>("128"), 127);

    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("-1"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("0"), 0U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("255"), 255U);
    BOOST_CHECK_EQUAL(LocaleIndependentAtoi<uint8_t>("256"), 255U);
}

BOOST_AUTO_TEST_CASE(test_ParseInt64)
{
    int64_t n;
    // Valid values
    BOOST_CHECK(ParseInt64("1234", nullptr));
    BOOST_CHECK(ParseInt64("0", &n) && n == 0LL);
    BOOST_CHECK(ParseInt64("1234", &n) && n == 1234LL);
    BOOST_CHECK(ParseInt64("01234", &n) && n == 1234LL); // no octal
    BOOST_CHECK(ParseInt64("2147483647", &n) && n == 2147483647LL);
    BOOST_CHECK(ParseInt64("-2147483648", &n) && n == -2147483648LL);
    BOOST_CHECK(ParseInt64("9223372036854775807", &n) && n == int64_t{9223372036854775807});
    BOOST_CHECK(ParseInt64("-9223372036854775808", &n) && n == int64_t{-9223372036854775807-1});
    BOOST_CHECK(ParseInt64("-1234", &n) && n == -1234LL);
    // Invalid values
    BOOST_CHECK(!ParseInt64("", &n));
    BOOST_CHECK(!ParseInt64(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseInt64("1 ", &n));
    BOOST_CHECK(!ParseInt64("1a", &n));
    BOOST_CHECK(!ParseInt64("aap", &n));
    BOOST_CHECK(!ParseInt64("0x1", &n)); // no hex
    BOOST_CHECK(!ParseInt64(STRING_WITH_EMBEDDED_NULL_CHAR, &n));
    // Overflow and underflow
    BOOST_CHECK(!ParseInt64("-9223372036854775809", nullptr));
    BOOST_CHECK(!ParseInt64("9223372036854775808", nullptr));
    BOOST_CHECK(!ParseInt64("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseInt64("32482348723847471234", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt8)
{
    uint8_t n;
    // Valid values
    BOOST_CHECK(ParseUInt8("255", nullptr));
    BOOST_CHECK(ParseUInt8("0", &n) && n == 0);
    BOOST_CHECK(ParseUInt8("255", &n) && n == 255);
    BOOST_CHECK(ParseUInt8("0255", &n) && n == 255); // no octal
    BOOST_CHECK(ParseUInt8("255", &n) && n == static_cast<uint8_t>(255));
    BOOST_CHECK(ParseUInt8("+255", &n) && n == 255);
    BOOST_CHECK(ParseUInt8("00000000000000000012", &n) && n == 12);
    BOOST_CHECK(ParseUInt8("00000000000000000000", &n) && n == 0);
    // Invalid values
    BOOST_CHECK(!ParseUInt8("-00000000000000000000", &n));
    BOOST_CHECK(!ParseUInt8("", &n));
    BOOST_CHECK(!ParseUInt8(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt8(" -1", &n));
    BOOST_CHECK(!ParseUInt8("++1", &n));
    BOOST_CHECK(!ParseUInt8("+-1", &n));
    BOOST_CHECK(!ParseUInt8("-+1", &n));
    BOOST_CHECK(!ParseUInt8("--1", &n));
    BOOST_CHECK(!ParseUInt8("-1", &n));
    BOOST_CHECK(!ParseUInt8("1 ", &n));
    BOOST_CHECK(!ParseUInt8("1a", &n));
    BOOST_CHECK(!ParseUInt8("aap", &n));
    BOOST_CHECK(!ParseUInt8("0x1", &n)); // no hex
    BOOST_CHECK(!ParseUInt8(STRING_WITH_EMBEDDED_NULL_CHAR, &n));
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt8("-255", &n));
    BOOST_CHECK(!ParseUInt8("256", &n));
    BOOST_CHECK(!ParseUInt8("-123", &n));
    BOOST_CHECK(!ParseUInt8("-123", nullptr));
    BOOST_CHECK(!ParseUInt8("256", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt16)
{
    uint16_t n;
    // Valid values
    BOOST_CHECK(ParseUInt16("1234", nullptr));
    BOOST_CHECK(ParseUInt16("0", &n) && n == 0);
    BOOST_CHECK(ParseUInt16("1234", &n) && n == 1234);
    BOOST_CHECK(ParseUInt16("01234", &n) && n == 1234); // no octal
    BOOST_CHECK(ParseUInt16("65535", &n) && n == static_cast<uint16_t>(65535));
    BOOST_CHECK(ParseUInt16("+65535", &n) && n == 65535);
    BOOST_CHECK(ParseUInt16("00000000000000000012", &n) && n == 12);
    BOOST_CHECK(ParseUInt16("00000000000000000000", &n) && n == 0);
    // Invalid values
    BOOST_CHECK(!ParseUInt16("-00000000000000000000", &n));
    BOOST_CHECK(!ParseUInt16("", &n));
    BOOST_CHECK(!ParseUInt16(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt16(" -1", &n));
    BOOST_CHECK(!ParseUInt16("++1", &n));
    BOOST_CHECK(!ParseUInt16("+-1", &n));
    BOOST_CHECK(!ParseUInt16("-+1", &n));
    BOOST_CHECK(!ParseUInt16("--1", &n));
    BOOST_CHECK(!ParseUInt16("-1", &n));
    BOOST_CHECK(!ParseUInt16("1 ", &n));
    BOOST_CHECK(!ParseUInt16("1a", &n));
    BOOST_CHECK(!ParseUInt16("aap", &n));
    BOOST_CHECK(!ParseUInt16("0x1", &n)); // no hex
    BOOST_CHECK(!ParseUInt16(STRING_WITH_EMBEDDED_NULL_CHAR, &n));
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt16("-65535", &n));
    BOOST_CHECK(!ParseUInt16("65536", &n));
    BOOST_CHECK(!ParseUInt16("-123", &n));
    BOOST_CHECK(!ParseUInt16("-123", nullptr));
    BOOST_CHECK(!ParseUInt16("65536", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt32)
{
    uint32_t n;
    // Valid values
    BOOST_CHECK(ParseUInt32("1234", nullptr));
    BOOST_CHECK(ParseUInt32("0", &n) && n == 0);
    BOOST_CHECK(ParseUInt32("1234", &n) && n == 1234);
    BOOST_CHECK(ParseUInt32("01234", &n) && n == 1234); // no octal
    BOOST_CHECK(ParseUInt32("2147483647", &n) && n == 2147483647);
    BOOST_CHECK(ParseUInt32("2147483648", &n) && n == uint32_t{2147483648});
    BOOST_CHECK(ParseUInt32("4294967295", &n) && n == uint32_t{4294967295});
    BOOST_CHECK(ParseUInt32("+1234", &n) && n == 1234);
    BOOST_CHECK(ParseUInt32("00000000000000001234", &n) && n == 1234);
    BOOST_CHECK(ParseUInt32("00000000000000000000", &n) && n == 0);
    // Invalid values
    BOOST_CHECK(!ParseUInt32("-00000000000000000000", &n));
    BOOST_CHECK(!ParseUInt32("", &n));
    BOOST_CHECK(!ParseUInt32(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt32(" -1", &n));
    BOOST_CHECK(!ParseUInt32("++1", &n));
    BOOST_CHECK(!ParseUInt32("+-1", &n));
    BOOST_CHECK(!ParseUInt32("-+1", &n));
    BOOST_CHECK(!ParseUInt32("--1", &n));
    BOOST_CHECK(!ParseUInt32("-1", &n));
    BOOST_CHECK(!ParseUInt32("1 ", &n));
    BOOST_CHECK(!ParseUInt32("1a", &n));
    BOOST_CHECK(!ParseUInt32("aap", &n));
    BOOST_CHECK(!ParseUInt32("0x1", &n)); // no hex
    BOOST_CHECK(!ParseUInt32(STRING_WITH_EMBEDDED_NULL_CHAR, &n));
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt32("-2147483648", &n));
    BOOST_CHECK(!ParseUInt32("4294967296", &n));
    BOOST_CHECK(!ParseUInt32("-1234", &n));
    BOOST_CHECK(!ParseUInt32("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseUInt32("32482348723847471234", nullptr));
}

BOOST_AUTO_TEST_CASE(test_ParseUInt64)
{
    uint64_t n;
    // Valid values
    BOOST_CHECK(ParseUInt64("1234", nullptr));
    BOOST_CHECK(ParseUInt64("0", &n) && n == 0LL);
    BOOST_CHECK(ParseUInt64("1234", &n) && n == 1234LL);
    BOOST_CHECK(ParseUInt64("01234", &n) && n == 1234LL); // no octal
    BOOST_CHECK(ParseUInt64("2147483647", &n) && n == 2147483647LL);
    BOOST_CHECK(ParseUInt64("9223372036854775807", &n) && n == 9223372036854775807ULL);
    BOOST_CHECK(ParseUInt64("9223372036854775808", &n) && n == 9223372036854775808ULL);
    BOOST_CHECK(ParseUInt64("18446744073709551615", &n) && n == 18446744073709551615ULL);
    // Invalid values
    BOOST_CHECK(!ParseUInt64("", &n));
    BOOST_CHECK(!ParseUInt64(" 1", &n)); // no padding inside
    BOOST_CHECK(!ParseUInt64(" -1", &n));
    BOOST_CHECK(!ParseUInt64("1 ", &n));
    BOOST_CHECK(!ParseUInt64("1a", &n));
    BOOST_CHECK(!ParseUInt64("aap", &n));
    BOOST_CHECK(!ParseUInt64("0x1", &n)); // no hex
    BOOST_CHECK(!ParseUInt64(STRING_WITH_EMBEDDED_NULL_CHAR, &n));
    // Overflow and underflow
    BOOST_CHECK(!ParseUInt64("-9223372036854775809", nullptr));
    BOOST_CHECK(!ParseUInt64("18446744073709551616", nullptr));
    BOOST_CHECK(!ParseUInt64("-32482348723847471234", nullptr));
    BOOST_CHECK(!ParseUInt64("-2147483648", &n));
    BOOST_CHECK(!ParseUInt64("-9223372036854775808", &n));
    BOOST_CHECK(!ParseUInt64("-1234", &n));
}

BOOST_AUTO_TEST_CASE(test_FormatParagraph)
{
    BOOST_CHECK_EQUAL(FormatParagraph("", 79, 0), "");
    BOOST_CHECK_EQUAL(FormatParagraph("test", 79, 0), "test");
    BOOST_CHECK_EQUAL(FormatParagraph(" test", 79, 0), " test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 79, 0), "test test");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 0), "test\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("testerde test", 4, 0), "testerde\ntest");
    BOOST_CHECK_EQUAL(FormatParagraph("test test", 4, 4), "test\n    test");

    // Make sure we don't indent a fully-new line following a too-long line ending
    BOOST_CHECK_EQUAL(FormatParagraph("test test\nabc", 4, 4), "test\n    test\nabc");

    BOOST_CHECK_EQUAL(FormatParagraph("This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length until it gets here", 79), "This_is_a_very_long_test_string_without_any_spaces_so_it_should_just_get_returned_as_is_despite_the_length\nuntil it gets here");

    // Test wrap length is exact
    BOOST_CHECK_EQUAL(FormatParagraph("a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p", 79), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\nf g h i j k l m n o p");
    // Indent should be included in length of lines
    BOOST_CHECK_EQUAL(FormatParagraph("x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg h i j k", 79, 4), "x\na b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5 6 7 8 9 a b c de\n    f g h i j k l m n o p q r s t u v w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e fg\n    h i j k");

    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string. This is a second sentence in the very long test string.", 79), "This is a very long test string. This is a second sentence in the very long\ntest string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third sentence in the very long test string.", 79), "This is a very long test string.\n\nThis is a second sentence in the very long test string. This is a third\nsentence in the very long test string.");
    BOOST_CHECK_EQUAL(FormatParagraph("Testing that normal newlines do not get indented.\nLike here.", 79), "Testing that normal newlines do not get indented.\nLike here.");
}

BOOST_AUTO_TEST_CASE(test_ToLower)
{
    BOOST_CHECK_EQUAL(ToLower('@'), '@');
    BOOST_CHECK_EQUAL(ToLower('A'), 'a');
    BOOST_CHECK_EQUAL(ToLower('Z'), 'z');
    BOOST_CHECK_EQUAL(ToLower('['), '[');
    BOOST_CHECK_EQUAL(ToLower(0), 0);
    BOOST_CHECK_EQUAL(ToLower('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToLower(""), "");
    BOOST_CHECK_EQUAL(ToLower("#HODL"), "#hodl");
    BOOST_CHECK_EQUAL(ToLower("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_ToUpper)
{
    BOOST_CHECK_EQUAL(ToUpper('`'), '`');
    BOOST_CHECK_EQUAL(ToUpper('a'), 'A');
    BOOST_CHECK_EQUAL(ToUpper('z'), 'Z');
    BOOST_CHECK_EQUAL(ToUpper('{'), '{');
    BOOST_CHECK_EQUAL(ToUpper(0), 0);
    BOOST_CHECK_EQUAL(ToUpper('\xff'), '\xff');

    BOOST_CHECK_EQUAL(ToUpper(""), "");
    BOOST_CHECK_EQUAL(ToUpper("#hodl"), "#HODL");
    BOOST_CHECK_EQUAL(ToUpper("\x00\xfe\xff"), "\x00\xfe\xff");
}

BOOST_AUTO_TEST_CASE(test_Capitalize)
{
    BOOST_CHECK_EQUAL(Capitalize(""), "");
    BOOST_CHECK_EQUAL(Capitalize("bitcoin"), "Bitcoin");
    BOOST_CHECK_EQUAL(Capitalize("\x00\xfe\xff"), "\x00\xfe\xff");
}

static std::string SpanToStr(const Span<const char>& span)
{
    return std::string(span.begin(), span.end());
}

BOOST_AUTO_TEST_CASE(test_spanparsing)
{
    using namespace spanparsing;
    std::string input;
    Span<const char> sp;
    bool success;

    // Const(...): parse a constant, update span to skip it if successful
    input = "MilkToastHoney";
    sp = input;
    success = Const("", sp); // empty
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "MilkToastHoney");

    success = Const("Milk", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "ToastHoney");

    success = Const("Bread", sp);
    BOOST_CHECK(!success);

    success = Const("Toast", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Honey");

    success = Const("Honeybadger", sp);
    BOOST_CHECK(!success);

    success = Const("Honey", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");

    // Func(...): parse a function call, update span to argument if successful
    input = "Foo(Bar(xy,z()))";
    sp = input;

    success = Func("FooBar", sp);
    BOOST_CHECK(!success);

    success = Func("Foo(", sp);
    BOOST_CHECK(!success);

    success = Func("Foo", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "Bar(xy,z())");

    success = Func("Bar", sp);
    BOOST_CHECK(success);
    BOOST_CHECK_EQUAL(SpanToStr(sp), "xy,z()");

    success = Func("xy", sp);
    BOOST_CHECK(!success);

    // Expr(...): return expression that span begins with, update span to skip it
    Span<const char> result;

    input = "(n*(n-1))/2";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(n*(n-1))/2");
    BOOST_CHECK_EQUAL(SpanToStr(sp), "");

    input = "foo,bar";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",bar");

    input = "(aaaaa,bbbbb()),c";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "(aaaaa,bbbbb())");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",c");

    input = "xyz)foo";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "xyz");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ")foo");

    input = "((a),(b),(c)),xxx";
    sp = input;
    result = Expr(sp);
    BOOST_CHECK_EQUAL(SpanToStr(result), "((a),(b),(c))");
    BOOST_CHECK_EQUAL(SpanToStr(sp), ",xxx");

    // Split(...): split a string on every instance of sep, return vector
    std::vector<Span<const char>> results;

    input = "xxx";
    results = Split(input, 'x');
    BOOST_CHECK_EQUAL(results.size(), 4U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");

    input = "one#two#three";
    results = Split(input, '-');
    BOOST_CHECK_EQUAL(results.size(), 1U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one#two#three");

    input = "one#two#three";
    results = Split(input, '#');
    BOOST_CHECK_EQUAL(results.size(), 3U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "one");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "two");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "three");

    input = "*foo*bar*";
    results = Split(input, '*');
    BOOST_CHECK_EQUAL(results.size(), 4U);
    BOOST_CHECK_EQUAL(SpanToStr(results[0]), "");
    BOOST_CHECK_EQUAL(SpanToStr(results[1]), "foo");
    BOOST_CHECK_EQUAL(SpanToStr(results[2]), "bar");
    BOOST_CHECK_EQUAL(SpanToStr(results[3]), "");
}

BOOST_AUTO_TEST_CASE(test_LogEscapeMessage)
{
    // ASCII and UTF-8 must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Valid log message貓"), "Valid log message貓");
    // Newlines must pass through unaltered.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("Message\n with newlines\n"), "Message\n with newlines\n");
    // Other control characters are escaped in C syntax.
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage("\x01\x7f Corrupted log message\x0d"), R"(\x01\x7f Corrupted log message\x0d)");
    // Embedded NULL characters are escaped too.
    const std::string NUL("O\x00O", 3);
    BOOST_CHECK_EQUAL(BCLog::LogEscapeMessage(NUL), R"(O\x00O)");
}

namespace {

struct Tracker
{
    //! Points to the original object (possibly itself) we moved/copied from
    const Tracker* origin;
    //! How many copies where involved between the original object and this one (moves are not counted)
    int copies{0};

    Tracker() noexcept : origin(this) {}
    Tracker(const Tracker& t) noexcept : origin(t.origin), copies(t.copies + 1) {}
    Tracker(Tracker&& t) noexcept : origin(t.origin), copies(t.copies) {}
    Tracker& operator=(const Tracker& t) noexcept
    {
        origin = t.origin;
        copies = t.copies + 1;
        return *this;
    }
};

}

BOOST_AUTO_TEST_CASE(message_sign)
{
    const std::array<unsigned char, 32> privkey_bytes = {
        // just some random data
        // derived address from this private key: 15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs
        0xD9, 0x7F, 0x51, 0x08, 0xF1, 0x1C, 0xDA, 0x6E,
        0xEE, 0xBA, 0xAA, 0x42, 0x0F, 0xEF, 0x07, 0x26,
        0xB1, 0xF8, 0x98, 0x06, 0x0B, 0x98, 0x48, 0x9F,
        0xA3, 0x09, 0x84, 0x63, 0xC0, 0x03, 0x28, 0x66
    };

    const std::string message = "Trust no one";

    const std::string expected_signature =
        "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=";

    CKey privkey;
    std::string generated_signature;

    BOOST_REQUIRE_MESSAGE(!privkey.IsValid(),
        "Confirm the private key is invalid");

    BOOST_CHECK_MESSAGE(!MessageSign(privkey, message, generated_signature),
        "Sign with an invalid private key");

    privkey.Set(privkey_bytes.begin(), privkey_bytes.end(), true);

    BOOST_REQUIRE_MESSAGE(privkey.IsValid(),
        "Confirm the private key is valid");

    BOOST_CHECK_MESSAGE(MessageSign(privkey, message, generated_signature),
        "Sign with a valid private key");

    BOOST_CHECK_EQUAL(expected_signature, generated_signature);
}

BOOST_AUTO_TEST_CASE(message_verify)
{
    BOOST_CHECK_EQUAL(
        MessageVerify(
            "invalid address",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_INVALID_ADDRESS);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "3B5fQsEXEaV8v6U3ejYc8XaKXAkyQj2MjV",
            "signature should be irrelevant",
            "message too"),
        MessageVerificationResult::ERR_ADDRESS_NO_KEY);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "invalid signature, not in base64 encoding",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_MALFORMED_SIGNATURE);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "1KqbBpLy5FARmTPD4VZnDDpYjkUvkr82Pm",
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=",
            "message should be irrelevant"),
        MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "I never signed this"),
        MessageVerificationResult::ERR_NOT_SIGNED);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "15CRxFdyRpGZLW9w8HnHvVduizdL5jKNbs",
            "IPojfrX2dfPnH26UegfbGQQLrdK844DlHq5157/P6h57WyuS/Qsl+h/WSVGDF4MUi4rWSswW38oimDYfNNUBUOk=",
            "Trust no one"),
        MessageVerificationResult::OK);

    BOOST_CHECK_EQUAL(
        MessageVerify(
            "11canuhp9X2NocwCq7xNrQYTmUgZAnLK3",
            "IIcaIENoYW5jZWxsb3Igb24gYnJpbmsgb2Ygc2Vjb25kIGJhaWxvdXQgZm9yIGJhbmtzIAaHRtbCeDZINyavx14=",
            "Trust me"),
        MessageVerificationResult::OK);
}

BOOST_AUTO_TEST_CASE(remove_prefix)
{
    BOOST_CHECK_EQUAL(RemovePrefix("./util/system.h", "./"), "util/system.h");
    BOOST_CHECK_EQUAL(RemovePrefixView("foo", "foo"), "");
    BOOST_CHECK_EQUAL(RemovePrefix("foo", "fo"), "o");
    BOOST_CHECK_EQUAL(RemovePrefixView("foo", "f"), "oo");
    BOOST_CHECK_EQUAL(RemovePrefix("foo", ""), "foo");
    BOOST_CHECK_EQUAL(RemovePrefixView("fo", "foo"), "fo");
    BOOST_CHECK_EQUAL(RemovePrefix("f", "foo"), "f");
    BOOST_CHECK_EQUAL(RemovePrefixView("", "foo"), "");
    BOOST_CHECK_EQUAL(RemovePrefix("", ""), "");
}

BOOST_AUTO_TEST_CASE(util_ParseByteUnits)
{
    auto noop = ByteUnit::NOOP;

    // no multiplier
    BOOST_CHECK_EQUAL(ParseByteUnits("1", noop).value(), 1);
    BOOST_CHECK_EQUAL(ParseByteUnits("0", noop).value(), 0);

    BOOST_CHECK_EQUAL(ParseByteUnits("1k", noop).value(), 1000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("1K", noop).value(), 1ULL << 10);

    BOOST_CHECK_EQUAL(ParseByteUnits("2m", noop).value(), 2'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("2M", noop).value(), 2ULL << 20);

    BOOST_CHECK_EQUAL(ParseByteUnits("3g", noop).value(), 3'000'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("3G", noop).value(), 3ULL << 30);

    BOOST_CHECK_EQUAL(ParseByteUnits("4t", noop).value(), 4'000'000'000'000ULL);
    BOOST_CHECK_EQUAL(ParseByteUnits("4T", noop).value(), 4ULL << 40);

    // check default multiplier
    BOOST_CHECK_EQUAL(ParseByteUnits("5", ByteUnit::K).value(), 5ULL << 10);

    // NaN
    BOOST_CHECK(!ParseByteUnits("", noop));
    BOOST_CHECK(!ParseByteUnits("foo", noop));

    // whitespace
    BOOST_CHECK(!ParseByteUnits("123m ", noop));
    BOOST_CHECK(!ParseByteUnits(" 123m", noop));

    // no +-
    BOOST_CHECK(!ParseByteUnits("-123m", noop));
    BOOST_CHECK(!ParseByteUnits("+123m", noop));

    // zero padding
    BOOST_CHECK_EQUAL(ParseByteUnits("020M", noop).value(), 20ULL << 20);

    // fractions not allowed
    BOOST_CHECK(!ParseByteUnits("0.5T", noop));

    // overflow
    BOOST_CHECK(!ParseByteUnits("18446744073709551615g", noop));

    // invalid unit
    BOOST_CHECK(!ParseByteUnits("1x", noop));
}

BOOST_AUTO_TEST_SUITE_END()
