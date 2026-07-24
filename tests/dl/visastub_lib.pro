QT      -= gui
TEMPLATE = lib
CONFIG  += c++11
TARGET   = visastub
DESTDIR  = $$PWD/build/lib

# Shared VISA test double so plugin .so files resolve viOpen/viWrite/... at load.
INCLUDEPATH += $$PWD/../visastub
HEADERS += $$PWD/../visastub/visa.h $$PWD/../visastub/visastub_api.h
SOURCES += $$PWD/../visastub/visastub.cpp
