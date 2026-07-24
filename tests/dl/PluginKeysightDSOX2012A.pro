QT      -= gui
TEMPLATE = lib
CONFIG  += plugin c++11
TARGET   = PluginKeysightDSOX2012A
DESTDIR  = $$PWD/build/bin/plugins

# Real plugin .so (Q_PLUGIN_METADATA ON) linked against the stub-backed core.
INCLUDEPATH += $$PWD/../../ScopeCore/include $$PWD/../visastub $$PWD/../../plugins/PluginKeysightDSOX2012A
LIBS        += -L$$PWD/build/lib -lScopeCore -lvisastub

HEADERS += $$PWD/../../plugins/PluginKeysightDSOX2012A/KeysightDSOX2012APlugin.h
SOURCES += $$PWD/../../plugins/PluginKeysightDSOX2012A/KeysightDSOX2012APlugin.cpp
