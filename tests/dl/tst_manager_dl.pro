QT       += core testlib
QT       -= gui
CONFIG   += console c++11 testcase
CONFIG   -= app_bundle
TARGET    = tst_manager_dl
DESTDIR   = $$PWD/build

# Manager/dynamic-load test: links the shared ScopeCore + VISA stub and loads
# the real plugin .so files via QPluginLoader.
INCLUDEPATH += $$PWD/../../ScopeCore/include $$PWD/../visastub
LIBS        += -L$$PWD/build/lib -lScopeCore -lvisastub

SOURCES += tst_manager_dl.cpp
