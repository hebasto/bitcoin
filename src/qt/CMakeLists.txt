# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# See:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)

set(qt_minimum_required_version 5.11.3)
set(qt_package ${WITH_GUI})

find_package(${qt_package} ${qt_minimum_required_version} REQUIRED
  COMPONENTS Widgets Network LinguistTools
  HINTS ${qt5_brew_prefix}
  PATH_SUFFIXES Qt5 # Required on OpenBSD systems.
)
message(STATUS "Found Qt: ${${qt_package}_DIR} (found suitable version \"${${qt_package}_VERSION}\", minimum required is \"${qt_minimum_required_version}\")")

file(GLOB ts_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} locale/*.ts)
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/locale)
qt5_add_translation(qm_files ${ts_files})

add_definitions(-DQT_NO_KEYWORDS -DQT_USE_QSTRINGBUILDER)

add_library(bitcoinqt STATIC EXCLUDE_FROM_ALL "")
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
    $<$<PLATFORM_ID:Windows>:winshutdownmonitor.cpp>
    $<$<PLATFORM_ID:Darwin>:macdockiconhandler.mm>
    $<$<PLATFORM_ID:Darwin>:macnotificationhandler.mm>
    $<$<PLATFORM_ID:Darwin>:macos_appnap.mm>
)
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_link_libraries(bitcoinqt
  PUBLIC
    ${qt_package}::Widgets
  PRIVATE
    rpc_client
    leveldb
    Boost::boost
    $<$<CXX_COMPILER_ID:MSVC>:shlwapi>
)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_link_libraries(bitcoinqt PRIVATE "-framework AppKit")
endif()
if(TARGET NATPMP::NATPMP)
  target_link_libraries(bitcoinqt PRIVATE NATPMP::NATPMP)
endif()

if(ENABLE_WALLET)
  target_sources(bitcoinqt
    PRIVATE
      addressbookpage.cpp
      addresstablemodel.cpp
      askpassphrasedialog.cpp
      coincontroldialog.cpp
      coincontroltreewidget.cpp
      createwalletdialog.cpp
      editaddressdialog.cpp
      openuridialog.cpp
      overviewpage.cpp
      paymentserver.cpp
      psbtoperationsdialog.cpp
      qrimagewidget.cpp
      receivecoinsdialog.cpp
      receiverequestdialog.cpp
      recentrequeststablemodel.cpp
      sendcoinsdialog.cpp
      sendcoinsentry.cpp
      signverifymessagedialog.cpp
      transactiondesc.cpp
      transactiondescdialog.cpp
      transactionfilterproxy.cpp
      transactionoverviewwidget.cpp
      transactionrecord.cpp
      transactiontablemodel.cpp
      transactionview.cpp
      walletcontroller.cpp
      walletframe.cpp
      walletmodel.cpp
      walletmodeltransaction.cpp
      walletview.cpp
  )
  target_link_libraries(bitcoinqt
    PRIVATE
      bitcoin_wallet
      ${qt_package}::Network
  )
endif()

add_executable(bitcoin-qt "")
target_sources(bitcoin-qt
  PRIVATE
    main.cpp
    ../init/bitcoin-qt.cpp
)
target_link_libraries(bitcoin-qt
  PRIVATE
    bitcoinqt
    bitcoin_node
)
