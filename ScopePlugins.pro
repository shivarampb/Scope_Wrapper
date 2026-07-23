TEMPLATE = subdirs
CONFIG  += ordered

# Top-level project. Mirrors the ELoad_R2 layout: the core library builds first,
# then each per-model plugin links against it. Built plugins land in bin/plugins
# and are discovered at runtime by CScopeManager::loadPlugins().

SUBDIRS = \
    ScopeCore/ScopeCore.pro \
    plugins/PluginKeysightDSOX2012A/PluginKeysightDSOX2012A.pro \
    plugins/PluginKeysightDSO7104B/PluginKeysightDSO7104B.pro \
    plugins/PluginKeysightDSOS204A/PluginKeysightDSOS204A.pro \
    plugins/PluginKeysightMSO6054A/PluginKeysightMSO6054A.pro \
    plugins/PluginTektronixMDO34/PluginTektronixMDO34.pro \
    plugins/PluginTektronixTDS2024C/PluginTektronixTDS2024C.pro \
    plugins/PluginTektronixTDS1012B/PluginTektronixTDS1012B.pro \
    plugins/PluginRohdeSchwarzRTM3004/PluginRohdeSchwarzRTM3004.pro \
    plugins/PluginRohdeSchwarzRTO2064/PluginRohdeSchwarzRTO2064.pro \
    plugins/PluginLeCroyWaveSurfer42XS/PluginLeCroyWaveSurfer42XS.pro
