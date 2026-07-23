// KeysightDSOS204APlugin.h — auto-structured per-model scope plugin (derives CVisaScopePlugin).
#ifndef KEYSIGHTDSOS204APLUGIN_H
#define KEYSIGHTDSOS204APLUGIN_H

#include <QObject>
#include "CVisaScopePlugin.h"

class CKeysightDSOS204APlugin : public QObject, public CVisaScopePlugin {
    Q_OBJECT
#ifndef SCOPE_NO_PLUGIN_METADATA
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
#endif
    Q_INTERFACES(CIScopePlugin)
public:
    CKeysightDSOS204APlugin();
};

#endif // KEYSIGHTDSOS204APLUGIN_H
