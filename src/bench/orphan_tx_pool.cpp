// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <net_processing.h>
#include <primitives/transaction.h>
#include <net.h>
#include <random.h>
#include <key.h>
#include <script/standard.h>
#include <script/signingprovider.h>

#include <vector>

static constexpr CAmount CENT{1000000};
struct COrphanTx {
    CTransactionRef tx;
    NodeId fromPeer;
    int64_t nTimeExpire;
};

extern bool AddOrphanTx(const CTransactionRef& tx, NodeId peer);
extern unsigned int EvictOrphanTxs(unsigned int nMaxOrphans);

static void OrphanTxPool(benchmark::State& state)
{
    const ECCVerifyHandle verify_handle;
    ECC_Start();

    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    FastRandomContext rand;

    std::vector<CTransactionRef> txs;
    for (unsigned int i = 0; i < DEFAULT_MAX_ORPHAN_TRANSACTIONS; ++i) {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = rand.rand256();
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1 * CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(pubkey));
        txs.emplace_back(MakeTransactionRef(tx));
    }

    while (state.KeepRunning()) {
        {
            LOCK(g_cs_orphans);
            for (size_t i = 0; i < txs.size(); ++i) {
                AddOrphanTx(txs.at(i), i);
            }
        }
        EvictOrphanTxs(0);
    }
}

BENCHMARK(OrphanTxPool, 10000);
