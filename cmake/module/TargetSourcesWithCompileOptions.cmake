# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Specifies sources to use when building a target, along
# with a list of additional options for those sources.
function(target_sources_with_compile_options target)
  cmake_parse_arguments(PARSE_ARGV 1 _ "" "" "COMPILE_OPTIONS")
  target_sources(${target} ${__UNPARSED_ARGUMENTS})
  set_property(SOURCE ${__UNPARSED_ARGUMENTS} PROPERTY COMPILE_OPTIONS ${__COMPILE_OPTIONS})
endfunction()
