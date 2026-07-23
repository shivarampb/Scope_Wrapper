TEMPLATE = subdirs
CONFIG  += ordered

# Top-level project. Mirrors the ELoad_R2 layout: the core library builds first,
# then each per-model plugin links against it. Built plugins land in bin/plugins
# and are discovered at runtime by CScopeManager::loadPlugins().

SUBDIRS = \
    ScopeCore/ScopeCore.pro \
    plugins/PluginKeysightDSOX2012A/PluginKeysightDSOX2012A.pro
