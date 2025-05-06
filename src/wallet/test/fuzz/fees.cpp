// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <wallet/coincontrol.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>
#include <validation.h>

namespace wallet {
namespace {
const TestingSetup* g_setup;
static std::unique_ptr<CWallet> g_wallet_ptr;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    const auto& node{g_setup->m_node};
    // g_wallet_ptr = std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase());
}

FUZZ_TARGET(wallet_fees, .init = initialize_setup)
{
}
} // namespace
} // namespace wallet
