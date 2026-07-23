// KeysightDSO7104BPlugin.h — auto-structured per-model scope plugin (derives CVisaScopePlugin).
#ifndef KEYSIGHTDSO7104BPLUGIN_H
#define KEYSIGHTDSO7104BPLUGIN_H

#include <QObject>
#include "CVisaScopePlugin.h"

class CKeysightDSO7104BPlugin : public QObject, public CVisaScopePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CKeysightDSO7104BPlugin();
};

#endif // KEYSIGHTDSO7104BPLUGIN_H
