QT -= gui

# ScopeCore - shared core library for the scope plugin system.
# Analogue of the ELoad_R2 PowerSupplyCore library.

TEMPLATE = lib
CONFIG  += c++11
TARGET   = ScopeCore
DESTDIR  = $$PWD/../lib

DEFINES += SCOPECORE_LIBRARY

INCLUDEPATH += $$PWD/include

HEADERS += \
    include/dp_types.h \
    include/ScopeTypes.h \
    include/ScopeError.h \
    include/VisaHelper.h \
    include/IScopePlugin.h \
    include/CScpiCommandBuilder.h \
    include/ScopeDialect.h \
    include/CVisaScopePlugin.h \
    include/ScopeFamilies.h \
    include/ScopeManager.h

SOURCES += \
    src/ScopeTypes.cpp \
    src/ScopeError.cpp \
    src/CScpiCommandBuilder.cpp \
    src/CVisaScopePlugin.cpp \
    src/ScopeFamilies.cpp \
    src/ScopeManager.cpp

# CVisaScopePlugin includes <visa.h>; expose VISA headers to the core build.
win32 {
    INCLUDEPATH += "C:/Program Files/IVI Foundation/VISA/Win64/Include"
    LIBS += -L"C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc" -lvisa64
}
unix {
    INCLUDEPATH += /usr/include
    LIBS += -lvisa
}
