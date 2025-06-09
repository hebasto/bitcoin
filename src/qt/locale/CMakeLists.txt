# Copyright (c) 2025-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(qm_files
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_am.qm
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_ar.qm
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_ast_ES.qm
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_ay.qm
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_az.qm
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_az@latin.qm
)

foreach(qm_file IN LISTS qm_files)
  string(REGEX REPLACE "^${CMAKE_CURRENT_BINARY_DIR}/bitcoin_(.*)\\.qm$" "\\1" locale "${qm_file}")
  set_property(SOURCE ${qm_file} PROPERTY QT_RESOURCE_ALIAS ${locale})
  message("============================ locale of ${qm_file}: ${locale}")
endforeach()

qt_add_resources(bitcoinqt gui_translations
  PREFIX /translations
  OUTPUT_TARGETS gui_translations_targets
  FILES ${qm_files}
)


message("============================ gui_translations_targets: ${gui_translations_targets}")  # bitcoinqt_resources_1


# 1. Source: build/src/qt/locale/.qt/rcc/qrc_gui_translations_init.cpp
# 2. Object: build/src/qt/locale/CMakeFiles/bitcoinqt_resources_1.dir/.qt/rcc/qrc_gui_translations_init.cpp.o
# 3. Linked to build/bin/bitcoin-qt
