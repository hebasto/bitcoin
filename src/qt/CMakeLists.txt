# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

function(import_plugins target)
  if(CMAKE_CROSSCOMPILING OR VCPKG_TARGET_TRIPLET MATCHES "-static")
    set(plugins Qt5::QMinimalIntegrationPlugin)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      list(APPEND plugins Qt5::QXcbIntegrationPlugin)
    elseif(WIN32)
      list(APPEND plugins Qt5::QWindowsIntegrationPlugin Qt5::QWindowsVistaStylePlugin)
    elseif(APPLE)
      list(APPEND plugins Qt5::QCocoaIntegrationPlugin Qt5::QMacStylePlugin)
    endif()
    qt5_import_plugins(${target}
        INCLUDE ${plugins}
        EXCLUDE_BY_TYPE imageformats iconengines
    )
  endif()
endfunction()

# See:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS forms)

set(qt_minimum_required_version 5.11.3)

set(qt_components Core Gui Widgets Network LinguistTools)

if(CMAKE_CROSSCOMPILING)
  # The find_package(Qt ...) function internally uses find_library()
  # calls for all dependencies to ensure their availability.
  # In turn, the find_library() inspects the well-known locations
  # on the file system; therefore, it must be able to find
  # platform-specific system libraries, for example:
  # /usr/x86_64-w64-mingw32/lib/libm.a or /usr/arm-linux-gnueabihf/lib/libm.a.
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
endif()

find_package(Qt5 ${qt_minimum_required_version} REQUIRED
  COMPONENTS ${qt_components}
  HINTS ${qt5_brew_prefix}
  PATH_SUFFIXES Qt5 # Required on OpenBSD systems.
)
unset(qt_components)
message(STATUS "Found Qt: ${Qt5_DIR} (found suitable version \"${Qt5_VERSION}\", minimum required is \"${qt_minimum_required_version}\")")
unset(qt_minimum_required_version)

# TODO: The file(GLOB ...) command should be replaced with an explicit
# file list. Such a change must be synced with the corresponding change
# to https://github.com/bitcoin-core/bitcoin-maintainer-tools/blob/main/update-translations.py
file(GLOB ts_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} locale/*.ts)
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/locale)
qt5_add_translation(qm_files ${ts_files})

configure_file(bitcoin_locale.qrc bitcoin_locale.qrc COPYONLY)

add_library(bitcoinqt STATIC EXCLUDE_FROM_ALL
  bantablemodel.cpp
  bitcoin.cpp
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
  bitcoin.qrc
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin_locale.qrc
)
target_compile_definitions(bitcoinqt
  PUBLIC
    QT_NO_KEYWORDS
    QT_USE_QSTRINGBUILDER
)
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
)
target_link_libraries(bitcoinqt
  PUBLIC
    Qt5::Widgets
  PRIVATE
    core_interface
    bitcoin_cli
    leveldb
    Boost::headers
    $<TARGET_NAME_IF_EXISTS:NATPMP::NATPMP>
    $<TARGET_NAME_IF_EXISTS:MiniUPnPc::MiniUPnPc>
    $<$<PLATFORM_ID:Darwin>:-framework\ AppKit>
    $<$<CXX_COMPILER_ID:MSVC>:shlwapi>
)

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
      Qt5::Network
  )
endif()

if(CMAKE_CROSSCOMPILING OR VCPKG_TARGET_TRIPLET MATCHES "-static")
  # We want to define static plugins to link ourselves, thus preventing
  # automatic linking against a "sane" set of default static plugins.
  qt5_import_plugins(bitcoinqt
      EXCLUDE_BY_TYPE bearer iconengines imageformats platforms styles
  )
endif()

add_executable(bitcoin-qt
  main.cpp
  ../init/bitcoin-qt.cpp
)

add_windows_resources(bitcoin-qt res/bitcoin-qt-res.rc)

target_link_libraries(bitcoin-qt
  core_interface
  bitcoinqt
  bitcoin_node
)

import_plugins(bitcoin-qt)
set(installable_targets bitcoin-qt)
if(WIN32)
  set_target_properties(bitcoin-qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(MULTIPROCESS)
  add_executable(bitcoin-gui
    main.cpp
    ../init/bitcoin-gui.cpp
  )
  target_link_libraries(bitcoin-gui
    core_interface
    bitcoinqt
    bitcoin_node
    bitcoin_ipc
  )
  import_plugins(bitcoin-gui)
  list(APPEND installable_targets bitcoin-gui)
  if(WIN32)
    set_target_properties(bitcoin-gui PROPERTIES WIN32_EXECUTABLE TRUE)
  endif()
endif()

install(TARGETS ${installable_targets}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT GUI
)
