QT      -= gui
TEMPLATE = lib
CONFIG  += plugin c++11
TARGET   = PluginTektronixMDO34
DESTDIR  = $$PWD/build/bin/plugins

# Real plugin .so (Q_PLUGIN_METADATA ON) linked against the stub-backed core.
INCLUDEPATH += $$PWD/../../ScopeCore/include $$PWD/../visastub $$PWD/../../plugins/PluginTektronixMDO34
LIBS        += -L$$PWD/build/lib -lScopeCore -lvisastub

HEADERS += $$PWD/../../plugins/PluginTektronixMDO34/TektronixMDO34Plugin.h
SOURCES += $$PWD/../../plugins/PluginTektronixMDO34/TektronixMDO34Plugin.cpp
