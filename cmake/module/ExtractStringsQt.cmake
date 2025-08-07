# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

find_program(XGETTEXT_EXECUTABLE xgettext)
find_program(SED_EXECUTABLE sed)

set(translatable_sources
  ${PROJECT_SOURCE_DIR}/src/common/init.cpp
  ${PROJECT_SOURCE_DIR}/src/common/messages.cpp
)

execute_process(
  COMMAND ${XGETTEXT_EXECUTABLE} --output=- --from-code=utf-8 -n --keyword=_ ${translatable_sources}
  OUTPUT_VARIABLE XGETTEXT_OUTPUT
  RESULT_VARIABLE XGETTEXT_STATUS
  ERROR_QUIET
)

if(NOT XGETTEXT_STATUS EQUAL 0)
  message(FATAL_ERROR "xgettext failed to extract translations.")
endif()


message("XGETTEXT_OUTPUT:\n${XGETTEXT_OUTPUT}")
message("XGETTEXT_STATUS: ${XGETTEXT_STATUS}")


# string(REGEX MATCHALL "msgid \"[^\n\"]*\"" MSGID_LINES "${XGETTEXT_OUTPUT}")
string(REGEX MATCHALL "msgid \"([^\"\\\\]|\\\\.)*\"" MSGID_LINES "${XGETTEXT_OUTPUT}")


foreach(M IN LISTS MSGID_LINES)
    string(REGEX REPLACE "^msgid " "" M_CLEAN "${M}")
    if(M_CLEAN STREQUAL [[""]])
      message("The M_CLEAN is empty: ${M_CLEAN}")
    else()
      message("M_CLEAN:\n${M_CLEAN}")
    endif()
endforeach()

message("MSGID_LINES:\n${MSGID_LINES}")
