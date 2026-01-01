#!/usr/bin/env python3
# File: test/functional/MRE.py

from test_framework.test_framework import BitcoinTestFramework

class MRE(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("OK")

if __name__ == '__main__':
    MRE(__file__).main()
