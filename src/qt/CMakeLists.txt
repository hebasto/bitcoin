# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# See:
#  - https://cmake.org/cmake/help/latest/manual/cmake-qt.7.html
#  - https://doc.qt.io/qt-5/cmake-manual.html
#  - https://doc.qt.io/qt-6/cmake-manual.html

if(${CMAKE_VERSION} VERSION_LESS 3.16)
  if(BUILD_GUI STREQUAL Qt6)
    message(FATAL_ERROR "CMake >= 3.16 is required to build the GUI with Qt 6.")
  elseif(BUILD_GUI STREQUAL ON)
    set(BUILD_GUI Qt5)
  endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms)
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)
set(QT_NO_CREATE_VERSIONLESS_TARGETS ON)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  execute_process(COMMAND brew --prefix qt@5 OUTPUT_VARIABLE qt5_brew_prefix ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(BUILD_GUI STREQUAL ON)
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core PATHS ${qt5_brew_prefix})
  set(qt_package Qt${QT_VERSION_MAJOR})
else()
  set(qt_package ${BUILD_GUI})
endif()

find_package(${qt_package} 5.11.3 REQUIRED COMPONENTS Widgets Network LinguistTools PATHS ${qt5_brew_prefix})
message(STATUS "Found Qt, version ${${qt_package}_VERSION}")

file(GLOB ts_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} locale/*.ts)
set_source_files_properties(${ts_files} PROPERTIES OUTPUT_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/locale)
if(${${qt_package}_VERSION_MAJOR} EQUAL 5)
  qt5_add_translation(qm_files ${ts_files})
else()
  qt6_add_translation(qm_files ${ts_files})
endif()

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
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_sources(bitcoinqt PRIVATE winshutdownmonitor.cpp)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_sources(bitcoinqt
    PRIVATE
      macdockiconhandler.mm
      macnotificationhandler.mm
      macos_appnap.mm
  )
  target_link_libraries(bitcoinqt PRIVATE "-framework AppKit")
endif()
target_include_directories(bitcoinqt
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>/src
)
target_link_libraries(bitcoinqt
  PUBLIC
    ${qt_package}::Widgets
  PRIVATE
    rpc_client
    leveldb
    Boost::headers
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
    PUBLIC
      ${qt_package}::Network
      bitcoin_wallet
  )
endif()

if(CMAKE_CROSSCOMPILING)
  target_compile_definitions(bitcoinqt PRIVATE QT_STATICPLUGIN)
  target_link_libraries(Qt5::Core
    INTERFACE
      ${CMAKE_PREFIX_PATH}/lib/libqtpcre2.a
      ${CMAKE_PREFIX_PATH}/lib/libqtlibpng.a
      ${CMAKE_PREFIX_PATH}/lib/libqtharfbuzz.a
  )
  if(CMAKE_SYSTEM_NAME STREQUAL Linux AND TARGET Qt5::QXcbIntegrationPlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_XCB)
    target_link_libraries(Qt5::Core
      INTERFACE
        -L${CMAKE_PREFIX_PATH}/lib
        freetype
        fontconfig
        xcb
        xcb-icccm
        xcb-image
        xcb-keysyms
        xcb-randr
        xcb-render
        xcb-render-util
        xcb-shape
        xcb-shm
        xcb-sync
        xcb-xfixes
        xcb-xinerama
        xcb-xkb
        xkbcommon
        xkbcommon-x11
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL Windows AND TARGET Qt5::QWindowsIntegrationPlugin AND TARGET Qt5::QWindowsVistaStylePlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_WINDOWS)
    target_link_libraries(Qt5::Core
      INTERFACE
        dwmapi
        imm32
        netapi32
        shlwapi
        userenv
        uxtheme
        version
        winmm
        wtsapi32
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin AND TARGET Qt5::QCocoaIntegrationPlugin AND TARGET Qt5::QMacStylePlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_COCOA)
    target_link_libraries(Qt5::Core
      INTERFACE
        "-framework AppKit"
        "-framework Carbon"
        "-framework CoreVideo"
        "-framework IOKit"
        "-framework IOSurface"
        "-framework Metal"
        "-framework QuartzCore"
        "-framework Security"
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL Android AND TARGET Qt5::QAndroidPlatformIntegrationPlugin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_ANDROID)
  endif()
endif()

add_executable(bitcoin-qt)
target_sources(bitcoin-qt
  PRIVATE
    main.cpp
    ../init/bitcoin-qt.cpp
)
target_link_libraries(bitcoin-qt
  PRIVATE
    bitcoinqt
    bitcoin_wallet
    bitcoin_node
    bitcoin_consensus
    bitcoin_common
    bitcoin_util
    bitcoin_crypto
)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_link_options(bitcoin-qt PRIVATE -static)
endif()
