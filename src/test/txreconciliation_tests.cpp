// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txreconciliation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(txreconciliation_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(PreRegisterPeerTest)
{
    TxReconciliationTracker tracker;

    auto [we_initiate_recon, we_respond_recon, recon_version, recon_salt] = tracker.PreRegisterPeer(0, true);
    assert(!we_initiate_recon);
    assert(we_respond_recon);
    assert(recon_version == 1); // RECON_VERSION in src/txreconciliation.cpp

    std::tie(we_initiate_recon, we_respond_recon, recon_version, recon_salt) = tracker.PreRegisterPeer(1, false);
    assert(we_initiate_recon);
    assert(!we_respond_recon);
}

BOOST_AUTO_TEST_CASE(RegisterPeerTest)
{
    TxReconciliationTracker tracker;
    const uint64_t salt = 0;

    // Prepare a peer for reconciliation.
    tracker.PreRegisterPeer(0, true);

    // Both roles are false, don't register.
    assert(!tracker.RegisterPeer(0, true, false, false, 1, salt));

    // Invalid roles for the given connection direction.
    assert(!tracker.RegisterPeer(0, true, false, true, 1, salt));
    assert(!tracker.RegisterPeer(0, false, true, false, 1, salt));

    // Invalid version.
    assert(!tracker.RegisterPeer(0, true, true, false, 0, salt));

    // Valid registration.
    assert(!tracker.IsPeerRegistered(0));
    assert(tracker.RegisterPeer(0, true, true, false, 1, salt));
    assert(tracker.IsPeerRegistered(0));

    // Reconciliation version is higher than ours, should be able to register.
    assert(!tracker.IsPeerRegistered(1));
    tracker.PreRegisterPeer(1, true);
    assert(tracker.RegisterPeer(1, true, true, false, 2, salt));
    assert(tracker.IsPeerRegistered(1));

    // Do not register if there were no pre-registration for the peer.
    assert(!tracker.RegisterPeer(100, true, true, false, 1, salt));
    assert(!tracker.IsPeerRegistered(100));
}

BOOST_AUTO_TEST_CASE(ForgetPeerTest)
{
    TxReconciliationTracker tracker;
    NodeId peer_id0 = 0;

    // Removing peer after pre-registring works and does not let to register the peer.
    tracker.PreRegisterPeer(peer_id0, true);
    tracker.ForgetPeer(peer_id0);
    assert(!tracker.RegisterPeer(peer_id0, true, true, false, 1, 1));

    // Removing peer after it is registered works.
    tracker.PreRegisterPeer(peer_id0, true);
    assert(!tracker.IsPeerRegistered(peer_id0));
    tracker.RegisterPeer(peer_id0, true, true, false, 1, 1);
    assert(tracker.IsPeerRegistered(peer_id0));
    tracker.ForgetPeer(peer_id0);
    assert(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_CASE(IsPeerRegisteredTest)
{
    TxReconciliationTracker tracker;
    NodeId peer_id0 = 0;

    assert(!tracker.IsPeerRegistered(peer_id0));
    tracker.PreRegisterPeer(peer_id0, true);
    assert(!tracker.IsPeerRegistered(peer_id0));

    assert(tracker.RegisterPeer(peer_id0, true, true, false, 1, 1));
    assert(tracker.IsPeerRegistered(peer_id0));

    tracker.ForgetPeer(peer_id0);
    assert(!tracker.IsPeerRegistered(peer_id0));
}

BOOST_AUTO_TEST_SUITE_END()