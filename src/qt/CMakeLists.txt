# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# See:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/forms")

find_package(Qt5 5.11.3 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)

file(GLOB TS_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "locale/*.ts")
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/locale")
qt5_add_translation(QM_FILES ${TS_FILES})

add_library(bitcoinqt STATIC EXCLUDE_FROM_ALL)
target_sources(bitcoinqt
  PRIVATE
    bantablemodel.cpp
    bitcoin.cpp
    bitcoin.qrc
    bitcoin_locale.qrc
    bitcoinaddressvalidator.cpp
    bitcoinamountfield.cpp
    bitcoingui.cpp
    bitcoinunits.cpp
    clientmodel.cpp
    csvmodelwriter.cpp
    guiutil.cpp
    initexecutor.cpp
    intro.cpp
    modaloverlay.cpp
    networkstyle.cpp
    notificator.cpp
    optionsdialog.cpp
    optionsmodel.cpp
    peertablemodel.cpp
    peertablesortproxy.cpp
    platformstyle.cpp
    qvalidatedlineedit.cpp
    qvaluecombobox.cpp
    rpcconsole.cpp
    splashscreen.cpp
    trafficgraphwidget.cpp
    utilitydialog.cpp
)
# TODO: Rename `node/ui_interface.h`, and drop `SKIP_AUTOUIC ON`.
set_property(
  SOURCE
    bitcoin.cpp
    bitcoingui.cpp
  PROPERTY
    SKIP_AUTOUIC ON
)
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
)
target_link_libraries(bitcoinqt
  PUBLIC
    Qt5::Widgets
    Qt5::Gui
    Qt5::Core
  PRIVATE
    rpc_client
    leveldb
)

add_executable(bitcoin-qt)
target_sources(bitcoin-qt
  PRIVATE
    main.cpp
    ../init/bitcoin-qt.cpp
)
target_link_libraries(bitcoin-qt
  PRIVATE
    bitcoinqt
    bitcoin_node
    bitcoin_consensus
    bitcoin_common
    bitcoin_util
    bitcoin_crypto
)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_sources(bitcoin-qt PRIVATE winshutdownmonitor.cpp)
  target_link_options(bitcoin-qt PRIVATE -static)
endif()
