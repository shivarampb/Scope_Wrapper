QT -= gui

TEMPLATE = lib
CONFIG  += plugin c++11

TARGET   = PluginKeysightDSO7104B
DESTDIR  = $$PWD/../../bin/plugins

INCLUDEPATH += $$PWD/../../ScopeCore/include
LIBS += -L$$PWD/../../lib -lScopeCore

win32 {
    INCLUDEPATH += "C:/Program Files/IVI Foundation/VISA/Win64/Include"
    LIBS += -L"C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc" -lvisa64
}
unix {
    INCLUDEPATH += /usr/include
    LIBS += -lvisa
}

HEADERS += KeysightDSO7104BPlugin.h
SOURCES += KeysightDSO7104BPlugin.cpp
