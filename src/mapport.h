// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MAPPORT_H
#define BITCOIN_MAPPORT_H

/** -upnp default */
#ifdef USE_UPNP
static const bool DEFAULT_UPNP = USE_UPNP;
#else
static const bool DEFAULT_UPNP = false;
#endif

enum class MapPort {
    NAT_PMP,
    UPNP
};

void StartMapPort(MapPort proto = MapPort::UPNP);
void InterruptMapPort();
void StopMapPort();

#endif // BITCOIN_MAPPORT_H
