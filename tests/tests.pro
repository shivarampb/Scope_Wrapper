QT       += core testlib
QT       -= gui
CONFIG   += console c++11 testcase
CONFIG   -= app_bundle
TARGET    = tst_scope

# Plugins are instantiated directly here, not loaded as .so, so suppress the
# per-plugin Q_PLUGIN_METADATA entry points (they would collide at link time).
DEFINES  += SCOPE_NO_PLUGIN_METADATA

# Build ScopeCore + representative plugins directly against the VISA simulator
# (no real VISA, no hardware). This is the host-free integration harness.

INCLUDEPATH += \
    $$PWD/../ScopeCore/include \
    $$PWD/visastub \
    $$PWD/../plugins/PluginKeysightDSOX2012A \
    $$PWD/../plugins/PluginTektronixMDO34 \
    $$PWD/../plugins/PluginRohdeSchwarzRTM3004 \
    $$PWD/../plugins/PluginLeCroyWaveSurfer42XS

HEADERS += \
    $$PWD/../ScopeCore/include/ScopeManager.h \
    $$PWD/../plugins/PluginKeysightDSOX2012A/KeysightDSOX2012APlugin.h \
    $$PWD/../plugins/PluginTektronixMDO34/TektronixMDO34Plugin.h \
    $$PWD/../plugins/PluginRohdeSchwarzRTM3004/RohdeSchwarzRTM3004Plugin.h \
    $$PWD/../plugins/PluginLeCroyWaveSurfer42XS/LeCroyWaveSurfer42XSPlugin.h

SOURCES += \
    tst_scope.cpp \
    visastub/visastub.cpp \
    $$PWD/../ScopeCore/src/ScopeError.cpp \
    $$PWD/../ScopeCore/src/ScopeTypes.cpp \
    $$PWD/../ScopeCore/src/CScpiCommandBuilder.cpp \
    $$PWD/../ScopeCore/src/CVisaScopePlugin.cpp \
    $$PWD/../ScopeCore/src/ScopeFamilies.cpp \
    $$PWD/../ScopeCore/src/ScopeManager.cpp \
    $$PWD/../plugins/PluginKeysightDSOX2012A/KeysightDSOX2012APlugin.cpp \
    $$PWD/../plugins/PluginTektronixMDO34/TektronixMDO34Plugin.cpp \
    $$PWD/../plugins/PluginRohdeSchwarzRTM3004/RohdeSchwarzRTM3004Plugin.cpp \
    $$PWD/../plugins/PluginLeCroyWaveSurfer42XS/LeCroyWaveSurfer42XSPlugin.cpp
