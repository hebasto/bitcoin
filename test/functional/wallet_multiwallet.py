#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet.

Verify that a bitcoind node can load multiple wallet files
"""
from decimal import Decimal
from threading import Thread
import os
import platform
import shutil
import stat

from test_framework.authproxy import JSONRPCException
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import ErrorMatch
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    ensure_for,
    get_rpc_proxy,
)

got_loading_error = False


def test_load_unload(node, name):
    global got_loading_error
    while True:
        if got_loading_error:
            return
        try:
            node.loadwallet(name)
            node.unloadwallet(name)
        except JSONRPCException as e:
            if e.error['code'] == -4 and 'Wallet already loading' in e.error['message']:
                got_loading_error = True
                return


class MultiWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.rpc_timeout = 120
        self.extra_args = [["-nowallet"], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def add_options(self, parser):
        self.add_wallet_options(parser)
        parser.add_argument(
            '--data_wallets_dir',
            default=os.path.join(os.path.dirname(os.path.realpath(__file__)), 'data/wallets/'),
            help='Test data with wallet directories (default: %(default)s)',
        )

    def run_test(self):
        node = self.nodes[0]

        data_dir = lambda *p: os.path.join(node.chain_path, *p)
        wallet_dir = lambda *p: data_dir('wallets', *p)
        wallet = lambda name: node.get_wallet_rpc(name)

        def wallet_file(name):
            if name == self.default_wallet_name:
                return wallet_dir(self.default_wallet_name, self.wallet_data_filename)
            if os.path.isdir(wallet_dir(name)):
                return wallet_dir(name, "wallet.dat")
            return wallet_dir(name)

        assert_equal(self.nodes[0].listwalletdir(), {'wallets': [{'name': self.default_wallet_name}]})

        # check wallet.dat is created
        self.stop_nodes()
        assert_equal(os.path.isfile(wallet_dir(self.default_wallet_name, self.wallet_data_filename)), True)

        print('=====================================================================================')

        self.start_node(0)
        os.mkdir(wallet_dir('no_access'))
        os.chmod(wallet_dir('no_access'), 0)
        try:
            with self.nodes[0].assert_debug_log(expected_msgs=['Error scanning']):
                walletlist = self.nodes[0].listwalletdir()['wallets']
        finally:
            # Need to ensure access is restored for cleanup
            os.chmod(wallet_dir('no_access'), stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)


if __name__ == '__main__':
    MultiWalletTest(__file__).main()
