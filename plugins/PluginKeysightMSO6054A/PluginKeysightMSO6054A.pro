QT -= gui

TEMPLATE = lib
CONFIG  += plugin c++11

TARGET   = PluginKeysightMSO6054A
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

HEADERS += KeysightMSO6054APlugin.h
SOURCES += KeysightMSO6054APlugin.cpp
