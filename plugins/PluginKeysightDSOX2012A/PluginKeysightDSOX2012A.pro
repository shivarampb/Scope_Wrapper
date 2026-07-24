QT -= gui

# Keysight/Agilent InfiniiVision DSOX2012A - 100 MHz, 2-channel oscilloscope.
# Per-model scope plugin. Analogue of the ELoad_R2 PluginTDKLambdaZUP36_6.pro.

TEMPLATE = lib
CONFIG  += plugin c++11

TARGET   = PluginKeysightDSOX2012A
DESTDIR  = $$PWD/../../bin/plugins

INCLUDEPATH += $$PWD/../../ScopeCore/include

LIBS += -L$$PWD/../../lib -lScopeCore

# VISA library
win32 {
    INCLUDEPATH += "C:/Program Files/IVI Foundation/VISA/Win64/Include"
    LIBS += -L"C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc" -lvisa64
}

unix {
    INCLUDEPATH += /usr/include
    LIBS += -lvisa
}

HEADERS += \
    KeysightDSOX2012APlugin.h

SOURCES += \
    KeysightDSOX2012APlugin.cpp
