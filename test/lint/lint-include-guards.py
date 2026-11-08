#!/usr/bin/env python3
#
# Copyright (c) 2018-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""
Check include guards.
"""

import re
import sys
from subprocess import check_output

from lint_ignore_dirs import SHARED_EXCLUDED_SUBTREES


HEADER_ID_PREFIX = 'BITCOIN_'
HEADER_ID_SUFFIX = '_H'

EXCLUDE_FILES_WITH_PREFIX = ['contrib/devtools/bitcoin-tidy',
                             'src/tinyformat.h',
                             'src/bench/nanobench.h',
                             'src/test/fuzz/FuzzedDataProvider.h'] + SHARED_EXCLUDED_SUBTREES


def _get_header_file_lst() -> list[str]:
    """ Helper function to get a list of header filepaths to be
        checked for include guards.
    """
    git_cmd_lst = ['git', 'ls-files', '--', '*.h']
    header_file_lst = check_output(
        git_cmd_lst, text=True).splitlines()

    header_file_lst = [hf for hf in header_file_lst
                       if not any(ef in hf for ef
                                  in EXCLUDE_FILES_WITH_PREFIX)]

    return header_file_lst


def _get_header_id(header_file: str) -> str:
    """ Helper function to get the header id from a header file
        string.

        eg: 'src/wallet/walletdb.h' -> 'BITCOIN_WALLET_WALLETDB_H'

    Args:
        header_file: Filepath to header file.

    Returns:
        The header id.
    """
    header_id_base = header_file.split('/')[1:]
    header_id_base = '_'.join(header_id_base)
    header_id_base = header_id_base.replace('.h', '').replace('-', '_')
    header_id_base = header_id_base.upper()

    header_id = f'{HEADER_ID_PREFIX}{header_id_base}{HEADER_ID_SUFFIX}'

    return header_id


def _is_comment_or_blank(line: str, in_block_comment: bool) -> tuple[bool, bool]:
    """ Helper function to tell whether a line carries no code.

    Args:
        line: A line of the header file.
        in_block_comment: Whether the previous line left a /* ... */ comment open.

    Returns:
        A pair (is_comment_or_blank, in_block_comment) where the second
        element is the block comment state after this line.
    """
    stripped = line.strip()
    if in_block_comment:
        return True, '*/' not in stripped
    if stripped == '' or stripped.startswith('//'):
        return True, False
    if stripped.startswith('/*'):
        return True, '*/' not in stripped
    return False, False


def main():
    exit_code = 0

    header_file_lst = _get_header_file_lst()
    for header_file in header_file_lst:
        header_id = _get_header_id(header_file)

        regex_pattern = f'^#(ifndef|define|endif //) {header_id}'

        with open(header_file, 'r') as f:
            header_file_contents = f.readlines()

        count = 0
        ifndef_line_num = None
        endif_line_num = None
        for line_num, header_file_contents_string in enumerate(header_file_contents, start=1):
            include_guard_lst = re.findall(
                regex_pattern, header_file_contents_string)

            count += len(include_guard_lst)
            if header_file_contents_string.startswith(f'#ifndef {header_id}'):
                ifndef_line_num = line_num
            elif header_file_contents_string.startswith(f'#endif // {header_id}'):
                endif_line_num = line_num

        if count != 3:
            print(f'{header_file} seems to be missing the expected '
                  'include guard to prevent the double inclusion problem:')
            print(f'  #ifndef {header_id}')
            print(f'  #define {header_id}')
            print('  ...')
            print(f'  #endif // {header_id}\n')
            exit_code = 1
            continue

        # Everything except comments must be inside the include guard.
        unguarded_line_nums = []
        in_block_comment = False
        for line_num, header_file_contents_string in enumerate(header_file_contents, start=1):
            is_comment_or_blank, in_block_comment = _is_comment_or_blank(
                header_file_contents_string, in_block_comment)
            if is_comment_or_blank:
                continue
            if line_num < ifndef_line_num or line_num > endif_line_num:
                unguarded_line_nums.append(line_num)

        if unguarded_line_nums:
            print(f'{header_file} has code outside the include guard '
                  f'{header_id} on line(s) '
                  f'{", ".join(str(n) for n in unguarded_line_nums)}. '
                  'Move it inside the guard.\n')
            exit_code = 1

    sys.exit(exit_code)


if __name__ == '__main__':
    main()
