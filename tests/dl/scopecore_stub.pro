QT      -= gui
TEMPLATE = lib
CONFIG  += c++11
TARGET   = ScopeCore
DESTDIR  = $$PWD/build/lib

# ScopeCore built as a shared library against the VISA stub, so plugins and the
# manager share one set of CIScopePlugin vtables/type-info (needed for
# qobject_cast across the QPluginLoader boundary).
INCLUDEPATH += $$PWD/../../ScopeCore/include $$PWD/../visastub
LIBS        += -L$$PWD/build/lib -lvisastub

HEADERS += $$PWD/../../ScopeCore/include/ScopeManager.h
SOURCES += \
    $$PWD/../../ScopeCore/src/ScopeTypes.cpp \
    $$PWD/../../ScopeCore/src/ScopeError.cpp \
    $$PWD/../../ScopeCore/src/CScpiCommandBuilder.cpp \
    $$PWD/../../ScopeCore/src/CVisaScopePlugin.cpp \
    $$PWD/../../ScopeCore/src/ScopeFamilies.cpp \
    $$PWD/../../ScopeCore/src/ScopeManager.cpp
