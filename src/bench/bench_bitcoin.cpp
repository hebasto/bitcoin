// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <iostream>
#include <ws2tcpip.h>

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};
const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS{};

static int test_ws2()
{
    addrinfo ai_hint{};
    ai_hint.ai_socktype = SOCK_STREAM;
    ai_hint.ai_protocol = IPPROTO_TCP;
    ai_hint.ai_family = AF_UNSPEC;
    ai_hint.ai_flags = AI_ADDRCONFIG;
    addrinfo* ai_res{nullptr};
    return getaddrinfo("bitcoincore.org", nullptr, &ai_hint, &ai_res);
}

int main(int argc, char** argv)
{
    std::cerr << test_ws2() << '\n';

    // Comment the next line out to trigger the WSANOTINITIALISED=10093 error.
    const auto test_setup = MakeNoLogFileContext<const TestingSetup>();

    std::cerr << test_ws2() << '\n';
    return EXIT_SUCCESS;
}
