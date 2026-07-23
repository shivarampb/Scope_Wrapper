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
    include/ScopeManager.h

SOURCES += \
    src/ScopeTypes.cpp \
    src/ScopeError.cpp \
    src/CScpiCommandBuilder.cpp \
    src/ScopeManager.cpp
