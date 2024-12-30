// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <util/translation.h>

#include <functional>
#include <iostream>
#include <optional>
#include <string>

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

MAIN_FUNCTION
{
    CNetAddr addr;
    const std::string link_local{"fe80::1"};
    addr = LookupHost(link_local + "%0", false).value();
    std::cerr << addr.ToStringAddr() << "\n";
    return 0;
}
