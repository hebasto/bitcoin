// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txreconciliation.h>

#include <node/minisketchwrapper.h>

#include <unordered_map>
#include <util/hasher.h>

namespace {

/** Current protocol version */
constexpr uint32_t RECON_VERSION = 1;
/** Static salt component used to compute short txids for sketch construction, see BIP-330. */
const std::string RECON_STATIC_SALT = "Tx Relay Salting";
/** Announce transactions via full wtxid to a limited number of inbound and outbound peers. */
constexpr double INBOUND_FANOUT_DESTINATIONS_FRACTION = 0.1;
constexpr double OUTBOUND_FANOUT_DESTINATIONS_FRACTION = 0.1;
/** The size of the field, used to compute sketches to reconcile transactions (see BIP-330). */
constexpr unsigned int RECON_FIELD_SIZE = 32;
/**
 * Limit sketch capacity to avoid DoS. This applies only to the original sketches,
 * and implies that extended sketches could be at most twice the size.
 */
constexpr uint32_t MAX_SKETCH_CAPACITY = 2 << 12;
/**
* It is possible that if sketch encodes more elements than the capacity, or
* if it is constructed of random bytes, sketch decoding may "succeed",
* but the result will be nonsense (false-positive decoding).
* Given this coef, a false positive probability will be of 1 in 2**coef.
*/
constexpr unsigned int RECON_FALSE_POSITIVE_COEF = 16;
static_assert(RECON_FALSE_POSITIVE_COEF <= 256,
    "Reducing reconciliation false positives beyond 1 in 2**256 is not supported");
/** Coefficient used to estimate reconciliation set differences. */
constexpr double RECON_Q = 0.25;
/**
  * Used to convert a floating point reconciliation coefficient q to integer for transmission.
  * Specified by BIP-330.
  */
constexpr uint16_t Q_PRECISION{(2 << 14) - 1};
/**
 * Interval between initiating reconciliations with peers.
 * This value allows to reconcile ~(7 tx/s * 8s) transactions during normal operation.
 * More frequent reconciliations would cause significant constant bandwidth overhead
 * due to reconciliation metadata (sketch sizes etc.), which would nullify the efficiency.
 * Less frequent reconciliations would introduce high transaction relay latency.
 */
constexpr std::chrono::microseconds RECON_REQUEST_INTERVAL{8s};
/**
 * We should keep an interval between responding to reconciliation requests from the same peer,
 * to reduce potential DoS surface.
 */
constexpr std::chrono::microseconds RECON_RESPONSE_INTERVAL{1s};

/**
 * Represents phase of the current reconciliation round with a peer.
 */
enum Phase {
    NONE,
    INIT_REQUESTED,
    INIT_RESPONDED,
};

/**
 * Salt (specified by BIP-330) constructed from contributions from both peers. It is used
 * to compute transaction short IDs, which are then used to construct a sketch representing a set
 * of transactions we want to announce to the peer.
 */
static uint256 ComputeSalt(uint64_t local_salt, uint64_t remote_salt)
{
    // Accoring to BIP-330, salts should be combined in ascending order.
    uint64_t salt1 = local_salt, salt2 = remote_salt;
    if (salt1 > salt2) std::swap(salt1, salt2);

    static const auto RECON_SALT_HASHER = TaggedHash(RECON_STATIC_SALT);
    return (CHashWriter(RECON_SALT_HASHER) << salt1 << salt2).GetSHA256();
}

/**
 * Keeps track of the transactions we want to announce to the peer along with the state
 * required to reconcile them.
 */
struct ReconciliationSet {
    /** Transactions we want to announce to the peer */
    std::set<uint256> m_wtxids;

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * This is a cache of these IDs enabling faster lookups of full wtxids,
     * useful when peer will ask for missing transactions by short IDs
     * at the end of a reconciliation round.
     */
    std::map<uint32_t, uint256> m_short_id_mapping;

    /** Get a number of transactions in the set. */
    size_t GetSize() const {
        return m_wtxids.size();
    }

    /** This should be called at the end of every reconciliation to avoid unbounded state growth. */
    void Clear() {
        m_wtxids.clear();
        m_short_id_mapping.clear();
    }

};

/**
 * Track ongoing reconciliations with a giving peer which were initiated by us.
 */
struct ReconciliationInitByUs {
    /** Keep track of the reconciliation phase with the peer. */
    Phase m_phase{Phase::NONE};
};

/**
 * Track ongoing reconciliations with a giving peer which were initiated by them.
 */
struct ReconciliationInitByThem {
    /**
     * The use of q coefficients is described above (see local_q comment).
     * The value transmitted from the peer with a reconciliation requests is stored here until
     * we respond to that request with a sketch.
     */
    double m_remote_q{RECON_Q};

    /**
     * A reconciliation request comes from a peer with a reconciliation set size from their side,
     * which is supposed to help us to estimate set difference size. The value is stored here until
     * we respond to that request with a sketch.
     */
    uint16_t m_remote_set_size;

    /**
     * We track when was the last time we responded to a reconciliation request by the peer,
     * so that we don't respond to them too often. This helps to reduce DoS surface.
     */
    std::chrono::microseconds m_last_init_recon_respond{0};
    /**
     * Returns whether at this time it's not too early to respond to a reconciliation request by
     * the peer, and, if so, bumps the time we last responded to allow further checks.
     */
    bool ConsiderInitResponseAndTrack() {
        auto current_time = GetTime<std::chrono::seconds>();
        if (m_last_init_recon_respond <= current_time - RECON_RESPONSE_INTERVAL) {
            m_last_init_recon_respond = current_time;
            return true;
        }
        return false;
    }

    /** Keep track of the reconciliation phase with the peer. */
    Phase m_phase{Phase::NONE};

    /**
     * Estimate a capacity of a sketch we will send or use locally (to find set difference)
     * based on the local set size.
     */
    uint32_t EstimateSketchCapacity(size_t local_set_size) const
    {
        const uint16_t set_size_diff = std::abs(uint16_t(local_set_size) - m_remote_set_size);
        const uint16_t min_size = std::min(uint16_t(local_set_size), m_remote_set_size);
        const uint16_t weighted_min_size = m_remote_q * min_size;
        const uint32_t estimated_diff = 1 + weighted_min_size + set_size_diff;
        return minisketch_compute_capacity(RECON_FIELD_SIZE, estimated_diff, RECON_FALSE_POSITIVE_COEF);
    }
};

/**
 * Used to keep track of the ongoing reconciliations, the transactions we want to announce to the
 * peer when next transaction reconciliation happens, and also all parameters required to perform
 * reconciliations.
 */
class ReconciliationState {

    /**
     * Reconciliation involves exchanging sketches, which efficiently represent transactions each
     * peer wants to announce. Sketches are computed over transaction short IDs.
     * These values are used to salt short IDs.
     */
    const uint64_t m_k0, m_k1;

    public:

    /**
     * Reconciliation protocol assumes using one role consistently: either a reconciliation
     * initiator (requesting sketches), or responder (sending sketches). This defines our role.
     * */
    const bool m_we_initiate;

    /**
     * Store all transactions which we would relay to the peer (policy checks passed, etc.)
     * in this set instead of announcing them right away. When reconciliation time comes, we will
     * compute an efficient representation of this set ("sketch") and use it to efficient reconcile
     * this set with a similar set on the other side of the connection.
     */
    ReconciliationSet m_local_set;

    /** Keep track of reconciliations with the peer. */
    ReconciliationInitByUs m_state_init_by_us;
    ReconciliationInitByThem m_state_init_by_them;

    ReconciliationState(uint64_t k0, uint64_t k1, bool we_initiate) :
        m_k0(k0), m_k1(k1), m_we_initiate(we_initiate) {}

    /**
     * Reconciliation sketches are computed over short transaction IDs.
     * Short IDs are salted with a link-specific constant value.
     */
    uint32_t ComputeShortID(const uint256 wtxid) const
    {
        const uint64_t s = SipHashUint256(m_k0, m_k1, wtxid);
        const uint32_t short_txid = 1 + (s & 0xFFFFFFFF);
        return short_txid;
    }

    /**
     * Reconciliation involves computing a space-efficient representation of transaction identifiers
     * (a sketch). A sketch has a capacity meaning it allows reconciling at most a certain number
     * of elements (see BIP-330).
     */
    Minisketch ComputeSketch(uint32_t& capacity)
    {
        Minisketch sketch;
        // Avoid serializing/sending an empty sketch.
        if (capacity == 0) return sketch;

        capacity = std::min(capacity, MAX_SKETCH_CAPACITY);
        sketch = MakeMinisketch32(capacity);

        for (const auto& wtxid: m_local_set.m_wtxids) {
            uint32_t short_txid = ComputeShortID(wtxid);
            sketch.Add(short_txid);
            m_local_set.m_short_id_mapping.emplace(short_txid, wtxid);
        }

        return sketch;
    }
};

} // namespace

/** Actual implementation for TxReconciliationTracker's data structure. */
class TxReconciliationTracker::Impl {

    mutable Mutex m_mutex;

    /**
     * We need a ReconciliationTracker-wide randomness to decide to which peers we should flood a
     * given transaction based on a (w)txid.
     */
    const SaltedTxidHasher txidHasher;


    /**
     * Per-peer salt is used to compute transaction short IDs, which will be later used to
     * construct reconciliation sketches.
     * Salt is generated randomly per-peer to prevent:
     * - linking of network nodes belonging to the same physical node
     * - halting of relay of particular transactions due to short ID collisions (DoS)
     */
    std::unordered_map<NodeId, uint64_t> m_local_salts GUARDED_BY(m_mutex);

    /**
     * Keeps track of ongoing reconciliations with a given peer.
     */
    std::unordered_map<NodeId, ReconciliationState> m_states GUARDED_BY(m_mutex);

    /**
     * Maintains a queue of reconciliations we should initiate. To achieve higher bandwidth
     * conservation and avoid overflows, we should reconcile in the same order, because then it’s
     * easier to estimate set difference size.
     */
    std::deque<NodeId> m_queue GUARDED_BY(m_mutex);

    /**
     * Make reconciliation requests periodically to make reconciliations efficient.
     */
    std::chrono::microseconds m_next_recon_request GUARDED_BY(m_mutex);
    void UpdateNextReconRequest(std::chrono::microseconds now) EXCLUSIVE_LOCKS_REQUIRED(m_mutex)
    {
        // We have one timer for the entire queue. This is safe because we initiate reconciliations
        // with outbound connections, which are unlikely to game this timer in a serious way.
        size_t we_initiate_to_count = std::count_if(m_states.begin(), m_states.end(),
            [](std::pair<NodeId, ReconciliationState> state) { return state.second.m_we_initiate; });
        m_next_recon_request = now + (RECON_REQUEST_INTERVAL / we_initiate_to_count);
    }

    public:

    std::tuple<bool, bool, uint32_t, uint64_t> PreRegisterPeer(NodeId peer_id, bool peer_inbound)
    {
        bool we_initiate_recon, we_respond_recon;
        // Currently reconciliation roles are defined by the connection direction: only the inbound
        // peer initiate reconciliations and the outbound peer is supposed to only respond.
        if (peer_inbound) {
            we_initiate_recon = false;
            we_respond_recon = true;
        } else {
            we_initiate_recon = true;
            we_respond_recon = false;
        }

        uint64_t m_local_recon_salt(GetRand(UINT64_MAX));
        bool added = WITH_LOCK(m_mutex, return m_local_salts.emplace(peer_id, m_local_recon_salt).second);
        // We do this exactly once per peer (which are unique by NodeId, see GetNewNodeId) so it's
        // safe to assume we don't have this record yet.
        assert(added);

        LogPrint(BCLog::NET, "Pre-register peer=%d for reconciling.\n", peer_id);
        return std::make_tuple(we_initiate_recon, we_respond_recon, RECON_VERSION, m_local_recon_salt);
    }

    bool RegisterPeer(NodeId peer_id, bool peer_inbound,
        bool they_may_initiate, bool they_may_respond, uint32_t recon_version, uint64_t remote_salt)
    {
        // We do not support reconciliation salt/version updates. We treat an attempt to update
        // after a successful registration as a protocol violation.
        LOCK(m_mutex);
        if (m_states.find(peer_id) != m_states.end()) return false;

        // If the peer supports the version which is lower than our, we downgrade to the version
        // they support. For now, this only guarantees that nodes with future reconciliation
        // versions have the choice of reconciling with this current version. However, they also
        // have the choice to refuse supporting reconciliations if the common version is not
        // satisfactory (e.g. too low).
        recon_version = std::min(recon_version, RECON_VERSION);
        // v1 is the lowest version, so suggesting something below must be a protocol violation.
        if (recon_version < 1) return false;

        auto local_salt = m_local_salts.find(peer_id);

        // This function should be called only after generating the local salt.
        if (local_salt == m_local_salts.end()) return false;

        // Must match SuggestReconciling logic.
        bool we_may_initiate = !peer_inbound, we_may_respond = peer_inbound;

        bool they_initiate = they_may_initiate && we_may_respond;
        bool we_initiate = we_may_initiate && they_may_respond;
        // If we ever announce we_initiate && we_may_respond, this will need tie-breaking. For now,
        // this is mutually exclusive because both are based on the inbound flag.
        assert(!(they_initiate && we_initiate));

        // The peer set both flags to false, we treat it as a protocol violation.
        if (!(they_initiate || we_initiate)) return false;

        if (we_initiate) {
            m_queue.push_back(peer_id);
        }

        LogPrint(BCLog::NET, "Register peer=%d for reconciling with the following params: " /* Continued */
            "we_initiate=%i, they_initiate=%i.\n", peer_id, we_initiate, they_initiate);

        uint256 full_salt = ComputeSalt(local_salt->second, remote_salt);

        assert(m_states.emplace(peer_id, ReconciliationState(full_salt.GetUint64(0),
            full_salt.GetUint64(1), we_initiate)).second);
        return true;
    }

    void AddToReconSet(NodeId peer_id, const std::vector<uint256>& txs_to_reconcile)
    {
        assert(txs_to_reconcile.size() > 0);
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        assert(recon_state != m_states.end());

        size_t added = 0;
        for (auto& wtxid: txs_to_reconcile) {
            if (recon_state->second.m_local_set.m_wtxids.insert(wtxid).second) {
                ++added;
            }
        }

        LogPrint(BCLog::NET, "Added %i new transactions to the reconciliation set for peer=%d. " /* Continued */
            "Now the set contains %i transactions.\n", added, peer_id, recon_state->second.m_local_set.GetSize());
    }

    void TryRemovingFromReconSet(NodeId peer_id, const uint256 wtxid_to_remove)
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) return;

        recon_state->second.m_local_set.m_wtxids.erase(wtxid_to_remove);
    }

    std::optional<std::pair<uint16_t, uint16_t>> MaybeRequestReconciliation(NodeId peer_id)
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) return std::nullopt;

        if (m_queue.size() > 0) {
            // Request transaction reconciliation periodically to efficiently exchange transactions.
            // To make reconciliation predictable and efficient, we reconcile with peers in order based on the queue,
            // and with a delay between requests.
            auto current_time = GetTime<std::chrono::seconds>();
            if (m_next_recon_request <= current_time && m_queue.front() == peer_id) {
                m_queue.pop_front();
                m_queue.push_back(peer_id);
                UpdateNextReconRequest(current_time);
                if (recon_state->second.m_state_init_by_us.m_phase != Phase::NONE) return std::nullopt;
                recon_state->second.m_state_init_by_us.m_phase = Phase::INIT_REQUESTED;

                size_t local_set_size = recon_state->second.m_local_set.GetSize();

                LogPrint(BCLog::NET, "Initiate reconciliation with peer=%d with the following params: " /* Continued */
                    "local_set_size=%i\n", peer_id, local_set_size);

                // In future, RECON_Q could be recomputed after every reconciliation based on the
                // set differences. For now, it provides good enough results without recompute
                // complexity, but we communicate it here to allow backward compatibility if
                // the value is changed or made dynamic.
                return std::make_pair(local_set_size, RECON_Q * Q_PRECISION);
            }
        }
        return std::nullopt;
    }

    void HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) return;
        if (recon_state->second.m_state_init_by_them.m_phase != Phase::NONE) return;
        if (recon_state->second.m_we_initiate) return;

        double peer_q_converted = peer_q * 1.0 / Q_PRECISION;
        recon_state->second.m_state_init_by_them.m_remote_q = peer_q_converted;
        recon_state->second.m_state_init_by_them.m_remote_set_size = peer_recon_set_size;
        recon_state->second.m_state_init_by_them.m_phase = Phase::INIT_REQUESTED;

        LogPrint(BCLog::NET, "Reconciliation initiated by peer=%d with the following params: " /* Continued */
            "remote_q=%d, remote_set_size=%i.\n", peer_id, peer_q_converted, peer_recon_set_size);
    }

    bool RespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata)
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) return false;
        if (recon_state->second.m_we_initiate) return false;

        Phase incoming_phase = recon_state->second.m_state_init_by_them.m_phase;

        // For initial requests we have an extra check to avoid short intervals between responses
        // to the same peer (see comments in the check function for justification).
        bool respond_to_initial_request = incoming_phase == Phase::INIT_REQUESTED &&
            recon_state->second.m_state_init_by_them.ConsiderInitResponseAndTrack();
        if (!respond_to_initial_request) {
            return false;
        }

        // Compute a sketch over the local reconciliation set.
        uint32_t sketch_capacity = 0;

        // We send an empty vector at initial request in the following 2 cases because
        // reconciliation can't help:
        // - if we have nothing on our side
        // - if they have nothing on their side
        // Then, they will terminate reconciliation early and force flooding-style announcement.
        if (recon_state->second.m_state_init_by_them.m_remote_set_size > 0 &&
                recon_state->second.m_local_set.GetSize() > 0) {

            sketch_capacity = recon_state->second.m_state_init_by_them.EstimateSketchCapacity(
                recon_state->second.m_local_set.GetSize());
            Minisketch sketch = recon_state->second.ComputeSketch(sketch_capacity);
            if (sketch) skdata = sketch.Serialize();
        }

        recon_state->second.m_state_init_by_them.m_phase = Phase::INIT_RESPONDED;

        LogPrint(BCLog::NET, "Responding with a sketch to reconciliation initiated by peer=%d: " /* Continued */
            "sending sketch of capacity=%i.\n", peer_id, sketch_capacity);

        return true;
    }

    void ForgetPeer(NodeId peer_id)
    {
        LOCK(m_mutex);
        auto salt_erased = m_local_salts.erase(peer_id);
        auto state_erased = m_states.erase(peer_id);
        if (salt_erased || state_erased) {
            LogPrint(BCLog::NET, "Forget reconciliation state of peer=%d.\n", peer_id);
        }
        m_queue.erase(std::remove(m_queue.begin(), m_queue.end(), peer_id), m_queue.end());
    }

    bool IsPeerRegistered(NodeId peer_id) const
    {
        LOCK(m_mutex);
        return m_states.find(peer_id) != m_states.end();
    }

    std::optional<size_t> GetPeerSetSize(NodeId peer_id) const
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) {
            return std::nullopt;
        }
        return recon_state->second.m_local_set.GetSize();
    }

    bool ShouldFloodTo(uint256 wtxid, NodeId peer_id) const
    {
        LOCK(m_mutex);

        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) {
            return false;
        }

        // In this function we make an assumption that reconciliation is always initiated from
        // inbound to outbound to avoid code complexity.
        std::vector<NodeId> eligible_peers;
        size_t flood_index_modulo;
        if (recon_state->second.m_we_initiate) {
            std::for_each(m_states.begin(), m_states.end(),
                [&eligible_peers](std::pair<NodeId, ReconciliationState> state) {
                    if (state.second.m_we_initiate) eligible_peers.push_back(state.first);
                }
            );
            flood_index_modulo = 1.0 / OUTBOUND_FANOUT_DESTINATIONS_FRACTION;
        } else {
            std::for_each(m_states.begin(), m_states.end(),
                [&eligible_peers](std::pair<NodeId, ReconciliationState> state) {
                    if (!state.second.m_we_initiate) eligible_peers.push_back(state.first);
                }
            );
            flood_index_modulo = 1.0 / INBOUND_FANOUT_DESTINATIONS_FRACTION;
        }

        const auto it = std::find(eligible_peers.begin(), eligible_peers.end(), peer_id);
        assert(it != eligible_peers.end());

        const size_t peer_index = it - eligible_peers.begin();
        return txidHasher(wtxid) % flood_index_modulo == peer_index % flood_index_modulo;
    }

    bool CurrentlyReconcilingTx(NodeId peer_id, const uint256 wtxid) const
    {
        LOCK(m_mutex);
        auto recon_state = m_states.find(peer_id);
        if (recon_state == m_states.end()) {
            return false;
        }
        return recon_state->second.m_local_set.m_wtxids.count(wtxid) > 0;
    }

};

TxReconciliationTracker::TxReconciliationTracker() :
    m_impl{std::make_unique<TxReconciliationTracker::Impl>()} {}

TxReconciliationTracker::~TxReconciliationTracker() = default;

std::tuple<bool, bool, uint32_t, uint64_t>TxReconciliationTracker::PreRegisterPeer(NodeId peer_id, bool peer_inbound)
{
    return m_impl->PreRegisterPeer(peer_id, peer_inbound);
}

bool TxReconciliationTracker::RegisterPeer(NodeId peer_id, bool peer_inbound,
    bool recon_requestor, bool recon_responder, uint32_t recon_version, uint64_t remote_salt)
{
    return m_impl->RegisterPeer(peer_id, peer_inbound, recon_requestor, recon_responder,
        recon_version, remote_salt);
}

void TxReconciliationTracker::AddToReconSet(NodeId peer_id, const std::vector<uint256>& txs_to_reconcile)
{
    m_impl->AddToReconSet(peer_id, txs_to_reconcile);
}

void TxReconciliationTracker::TryRemovingFromReconSet(NodeId peer_id, const uint256 wtxid_to_remove)
{
    m_impl->TryRemovingFromReconSet(peer_id, wtxid_to_remove);
}

std::optional<std::pair<uint16_t, uint16_t>> TxReconciliationTracker::MaybeRequestReconciliation(NodeId peer_id)
{
    return m_impl->MaybeRequestReconciliation(peer_id);
}

void TxReconciliationTracker::HandleReconciliationRequest(NodeId peer_id, uint16_t peer_recon_set_size, uint16_t peer_q)
{
    m_impl->HandleReconciliationRequest(peer_id, peer_recon_set_size, peer_q);
}

bool TxReconciliationTracker::RespondToReconciliationRequest(NodeId peer_id, std::vector<uint8_t>& skdata)
{
    return m_impl->RespondToReconciliationRequest(peer_id, skdata);
}

void TxReconciliationTracker::ForgetPeer(NodeId peer_id)
{
    m_impl->ForgetPeer(peer_id);
}

bool TxReconciliationTracker::IsPeerRegistered(NodeId peer_id) const
{
    return m_impl->IsPeerRegistered(peer_id);
}

std::optional<size_t> TxReconciliationTracker::GetPeerSetSize(NodeId peer_id) const
{
    return m_impl->GetPeerSetSize(peer_id);
}

bool TxReconciliationTracker::ShouldFloodTo(uint256 wtxid, NodeId peer_id) const
{
    return m_impl->ShouldFloodTo(wtxid, peer_id);
}

bool TxReconciliationTracker::CurrentlyReconcilingTx(NodeId peer_id, const uint256 wtxid) const
{
    return m_impl->CurrentlyReconcilingTx(peer_id, wtxid);
}
