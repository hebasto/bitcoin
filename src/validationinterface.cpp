// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validationinterface.h>

#include <attributes.h>
#include <chain.h>
#include <consensus/validation.h>
#include <kernel/chain.h>
#include <kernel/mempool_entry.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <scheduler.h>

#include <future>
#include <unordered_map>
#include <utility>

std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept;

void MainSignalsImpl::Register(std::shared_ptr<CValidationInterface> callbacks) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    auto inserted = m_map.emplace(callbacks.get(), m_list.end());
    if (inserted.second) inserted.first->second = m_list.emplace(m_list.end());
    inserted.first->second->callbacks = std::move(callbacks);
}

void MainSignalsImpl::Unregister(CValidationInterface* callbacks) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    auto it = m_map.find(callbacks);
    if (it != m_map.end()) {
        if (!--it->second->count) m_list.erase(it->second);
        m_map.erase(it);
    }
}

void MainSignalsImpl::Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    LOCK(m_mutex);
    for (const auto& entry : m_map) {
        if (!--entry.second->count) m_list.erase(entry.second);
    }
    m_map.clear();
}

template<typename F> void MainSignalsImpl::Iterate(F&& f) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
{
    WAIT_LOCK(m_mutex, lock);
    for (auto it = m_list.begin(); it != m_list.end();) {
        ++it->count;
        {
            REVERSE_LOCK(lock);
            f(*it->callbacks);
        }
        it = --it->count ? std::next(it) : m_list.erase(it);
    }
}

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler)
{
    assert(!m_internals);
    m_internals = std::make_unique<MainSignalsImpl>(scheduler);
}

void CMainSignals::UnregisterBackgroundSignalScheduler()
{
    m_internals.reset(nullptr);
}

void CMainSignals::FlushBackgroundCallbacks()
{
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CMainSignals::CallbacksPending()
{
    if (!m_internals) return 0;
    return m_internals->m_schedulerClient.CallbacksPending();
}

void CMainSignals::RegisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks)
{
    // Each connection captures the shared_ptr to ensure that each callback is
    // executed before the subscriber is destroyed. For more details see #18338.
    m_internals->Register(std::move(callbacks));
}

void CMainSignals::RegisterValidationInterface(CValidationInterface* callbacks)
{
    // Create a shared_ptr with a no-op deleter - CValidationInterface lifecycle
    // is managed by the caller.
    RegisterSharedValidationInterface({callbacks, [](CValidationInterface*){}});
}

void CMainSignals::UnregisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks)
{
    UnregisterValidationInterface(callbacks.get());
}

void CMainSignals::UnregisterValidationInterface(CValidationInterface* callbacks)
{
    if (m_internals) {
        m_internals->Unregister(callbacks);
    }
}

void CMainSignals::UnregisterAllValidationInterfaces()
{
    if (!m_internals) {
        return;
    }
    m_internals->Clear();
}

void CMainSignals::CallFunctionInValidationInterfaceQueue(std::function<void()> func)
{
    m_internals->m_schedulerClient.AddToProcessQueue(std::move(func));
}

void CMainSignals::SyncWithValidationInterfaceQueue()
{
    AssertLockNotHeld(cs_main);
    // Block until the validation queue drains
    std::promise<void> promise;
    CallFunctionInValidationInterfaceQueue([&promise] {
        promise.set_value();
    });
    promise.get_future().wait();
}

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging is not enabled.
//
// NOTE: The lambda captures all local variables by value.
#define ENQUEUE_AND_LOG_EVENT(event, fmt, name, ...)           \
    do {                                                       \
        auto local_name = (name);                              \
        LOG_EVENT("Enqueuing " fmt, local_name, __VA_ARGS__);  \
        m_internals->m_schedulerClient.AddToProcessQueue([=] { \
            LOG_EVENT(fmt, local_name, __VA_ARGS__);           \
            event();                                           \
        });                                                    \
    } while (0)

#define LOG_EVENT(fmt, ...) \
    LogPrint(BCLog::VALIDATION, fmt "\n", __VA_ARGS__)

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    // Dependencies exist that require UpdatedBlockTip events to be delivered in the order in which
    // the chain actually updates. One way to ensure this is for the caller to invoke this signal
    // in the same critical section where the chain is updated

    auto event = [pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: new block hash=%s fork block hash=%s (in IBD=%s)", __func__,
                          pindexNew->GetBlockHash().ToString(),
                          pindexFork ? pindexFork->GetBlockHash().ToString() : "null",
                          fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence)
{
    auto event = [tx, mempool_sequence, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.TransactionAddedToMempool(tx, mempool_sequence); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s wtxid=%s", __func__,
                          tx.info.m_tx->GetHash().ToString(),
                          tx.info.m_tx->GetWitnessHash().ToString());
}

void CMainSignals::TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {
    auto event = [tx, reason, mempool_sequence, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.TransactionRemovedFromMempool(tx, reason, mempool_sequence); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s wtxid=%s reason=%s", __func__,
                          tx->GetHash().ToString(),
                          tx->GetWitnessHash().ToString(),
                          RemovalReasonToString(reason));
}

void CMainSignals::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex) {
    auto event = [role, pblock, pindex, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockConnected(role, pblock, pindex); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight)
{
    auto event = [txs_removed_for_block, nBlockHeight, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.MempoolTransactionsRemovedForBlock(txs_removed_for_block, nBlockHeight); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block height=%s txs removed=%s", __func__,
                          nBlockHeight,
                          txs_removed_for_block.size());
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    auto event = [pblock, pindex, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockDisconnected(pblock, pindex); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::ChainStateFlushed(ChainstateRole role, const CBlockLocator &locator) {
    auto event = [role, locator, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.ChainStateFlushed(role, locator); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s", __func__,
                          locator.IsNull() ? "null" : locator.vHave.front().ToString());
}

void CMainSignals::BlockChecked(const CBlock& block, const BlockValidationState& state) {
    LOG_EVENT("%s: block hash=%s state=%s", __func__,
              block.GetHash().ToString(), state.ToString());
    m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockChecked(block, state); });
}

void CMainSignals::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    LOG_EVENT("%s: block hash=%s", __func__, block->GetHash().ToString());
    m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.NewPoWValidBlock(pindex, block); });
}
