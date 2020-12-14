# Create the super cache so modules will add themselves to it.
cache(, super)

TEMPLATE = subdirs
SUBDIRS = qtbase

SUBDIRS += bitcoincore_corelib
bitcoincore_corelib.subdir = qtbase/src/corelib
bitcoincore_corelib.depends = qtbase

SUBDIRS += bitcoincore_gui
bitcoincore_gui.subdir = qtbase/src/gui
bitcoincore_gui.depends = bitcoincore_corelib

SUBDIRS += bitcoincore_widgets
bitcoincore_widgets.subdir = qtbase/src/widgets
bitcoincore_widgets.depends = bitcoincore_corelib bitcoincore_gui

SUBDIRS += bitcoincore_network
bitcoincore_network.subdir = qtbase/src/network
bitcoincore_network.depends = bitcoincore_corelib

SUBDIRS += bitcoincore_plugins
bitcoincore_plugins.subdir = qtbase/src/plugins
bitcoincore_plugins.depends = bitcoincore_corelib bitcoincore_gui

SUBDIRS += bitcoincore_testlib
bitcoincore_testlib.subdir = qtbase/src/testlib
bitcoincore_testlib.depends = bitcoincore_corelib

SUBDIRS += bitcoincore_lrelease
bitcoincore_lrelease.subdir = qttools/src/linguist/lrelease
bitcoincore_lrelease.depends = bitcoincore_widgets

SUBDIRS += bitcoincore_lupdate
bitcoincore_lupdate.subdir = qttools/src/linguist/lupdate
bitcoincore_lupdate.depends = bitcoincore_widgets

SUBDIRS += qttranslations
qttranslations.depends = bitcoincore_lrelease bitcoincore_lupdate bitcoincore_widgets

load(qt_configure)
