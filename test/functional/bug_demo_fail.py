#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework

class BugDemoFail(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        self.stop_node(0)
        self.start_node(0)
        self.nodes[0].createwallet(wallet_name='w42')
        self.stop_node(0)

if __name__ == '__main__':
    BugDemoFail(__file__).main()
