# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Recursively collects source files from the given directories
# to be parsed for translatable strings.
set(result "")
foreach(directory IN ITEMS ${DIRECTORIES})
  file(GLOB_RECURSE sources
    ${directory}/*.h
    ${directory}/*.cpp
    ${directory}/*.mm
  )
  list(APPEND result ${sources})
endforeach()
set(subtrees crc32c crypto/ctaes leveldb minisketch secp256k1)
set(exclude_dirs bench compat crypto support test univalue)
foreach(directory IN LISTS subtrees exclude_dirs)
  list(FILTER result EXCLUDE REGEX ".*/src/${directory}/.*")
endforeach()
string(JOIN " " output ${result})
message("${output}")
