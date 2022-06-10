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

# if(CMAKE_SYSTEM_NAME STREQUAL Windows)
  target_link_libraries(Qt5::Core
    INTERFACE
      ${CMAKE_PREFIX_PATH}/lib/libqtpcre2.a
      # userenv
      # version
      # netapi32
      # winmm
      # shlwapi
      # "-framework DiskArbitration"
      "-framework IOKit"  # 350
      "-framework AppKit" # 3
      "-framework Security" #0
      "-framework Metal"  # 114
      "-framework CoreGraphics" # 114
      "-framework CoreServices"
  )
  target_link_libraries(Qt5::Gui
    INTERFACE
      ${CMAKE_PREFIX_PATH}/lib/libqtlibpng.a
      ${CMAKE_PREFIX_PATH}/lib/libqtharfbuzz.a
      # dwmapi
      # imm32
      # wtsapi32
      "-framework CoreFoundation"
  )
  target_link_libraries(Qt5::Widgets
    INTERFACE
      # uxtheme
      "-framework CoreFoundation"
  )
# endif()

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
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  target_sources(bitcoinqt
    PRIVATE
      macdockiconhandler.mm
      macnotificationhandler.mm
      macos_appnap.mm
)
endif()
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
if(CMAKE_CROSSCOMPILING)
  target_compile_definitions(bitcoinqt PRIVATE QT_STATICPLUGIN)
  if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_XCB)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_WINDOWS)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    target_compile_definitions(bitcoinqt PRIVATE QT_QPA_PLATFORM_COCOA)
  elseif(CMAKE_SYSTEM_NAME STREQUAL Android)
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
