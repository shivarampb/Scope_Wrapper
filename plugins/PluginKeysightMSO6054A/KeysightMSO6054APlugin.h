// KeysightMSO6054APlugin.h — auto-structured per-model scope plugin (derives CVisaScopePlugin).
#ifndef KEYSIGHTMSO6054APLUGIN_H
#define KEYSIGHTMSO6054APLUGIN_H

#include <QObject>
#include "CVisaScopePlugin.h"

class CKeysightMSO6054APlugin : public QObject, public CVisaScopePlugin {
    Q_OBJECT
#ifndef SCOPE_NO_PLUGIN_METADATA
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
#endif
    Q_INTERFACES(CIScopePlugin)
public:
    CKeysightMSO6054APlugin();
};

#endif // KEYSIGHTMSO6054APLUGIN_H
