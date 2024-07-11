# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include(${CMAKE_CURRENT_LIST_DIR}/CoverageIncludeBeforeTests.cmake)

set(functional_test_runner test/functional/test_runner.py)
if(EXTENDED_FUNCTIONAL_TESTS)
  list(APPEND functional_test_runner --extended)
endif()
if(DEFINED JOBS)
  list(APPEND CMAKE_CTEST_COMMAND -j ${JOBS})
  list(APPEND functional_test_runner -j ${JOBS})
endif()

# Run the tests.

execute_process(
  COMMAND ${CMAKE_CTEST_COMMAND}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
  COMMAND ${functional_test_runner}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND_ERROR_IS_FATAL ANY
)

include(${CMAKE_CURRENT_LIST_DIR}/CoverageIncludeAfterTests.cmake)
