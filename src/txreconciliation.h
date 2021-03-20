// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXRECONCILIATION_H
#define BITCOIN_TXRECONCILIATION_H

#include <net.h>
#include <sync.h>

#include <memory>
#include <tuple>

/**
 * Transaction reconciliation is a way for nodes to efficiently announce transactions.
 * This object keeps track of all reconciliation-related communications with the peers.
 * The high-level protocol is:
 * 0. Reconciliation protocol handshake.
 * 1. Once we receive a new transaction, add it to the set instead of announcing immediately
 * 2. When the time comes, a reconciliation initiator requests a sketch from the peer, where a sketch
 *    is a compressed representation of their set
 * 3. Once the initiator received a sketch from the peer, the initiator computes a local sketch,
 *    and combines the two skethes to find the difference in *sets*.
 * 4. Now the initiator knows full symmetrical difference and can request what the initiator is
 *    missing and announce to the peer what the peer is missing. For the former, an extra round is
 *    required because the initiator knows only short IDs of those transactions.
 * 5. Sometimes reconciliation fails if the difference is larger than the parties estimated,
 *    then there is one sketch extension round, in which the initiator requests for extra data.
 * 6. If extension succeeds, go to step 4.
 * 7. If extension fails, the initiator notifies the peer and announces all transactions from the
 *    corresponding set. Once the peer received the failure notification, the peer announces all
 *    transactions from the corresponding set.
 *
 * This is a modification of the Erlay protocol (https://arxiv.org/abs/1905.10518) with two
 * changes (sketch extensions instead of bisections, and an extra INV exchange round), both
 * are motivated in BIP-330.
 */
class TxReconciliationTracker {
    // Avoid littering this header file with implementation details.
    class Impl;
    const std::unique_ptr<Impl> m_impl;

    public:

    explicit TxReconciliationTracker();
    ~TxReconciliationTracker();

    /**
     * Step 0. Generates initial part of the state required to reconcile with the peer.
     * Returns the following values used to invite the peer to reconcile:
     * - whether we want to initiate reconciliation requests
     * - whether we agree to respond to reconciliation requests
     * - reconciliation protocol version
     * - salt used for short ID computation required for reconciliation
     * Reconciliation roles depend on whether the peer is inbound or outbound in this connection.
     * A peer can't participate in future reconciliations without this call.
     * This function must be called only once per peer.
     */
    std::tuple<bool, bool, uint32_t, uint64_t> PreRegisterPeer(NodeId peer_id, bool peer_inbound);

    /**
     * Step 0. Once the peer agreed to reconcile with us, generate the state required to track
     * ongoing reconciliations. Should be called only after pre-registering the peer and only once.
     * Does nothing and returns false if the peer violates the protocol.
     */
    bool RegisterPeer(NodeId peer_id, bool peer_inbound,
        bool recon_requestor, bool recon_responder, uint32_t recon_version, uint64_t remote_salt);

    // Helpers

    /**
     * Attempts to forget reconciliation-related state of the peer (if we previously stored any).
     * After this, we won't be able to reconcile with the peer.
     */
    void ForgetPeer(NodeId peer_id);

    /**
     * Check if a peer is registered to reconcile with us.
     */
    bool IsPeerRegistered(NodeId peer_id) const;
};

#endif // BITCOIN_TXRECONCILIATION_H
