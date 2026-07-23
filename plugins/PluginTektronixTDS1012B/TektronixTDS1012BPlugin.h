// TektronixTDS1012BPlugin.h — auto-structured per-model scope plugin (derives CTektronixScopeBase).
#ifndef TEKTRONIXTDS1012BPLUGIN_H
#define TEKTRONIXTDS1012BPLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CTektronixTDS1012BPlugin : public QObject, public CTektronixScopeBase {
    Q_OBJECT
#ifndef SCOPE_NO_PLUGIN_METADATA
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
#endif
    Q_INTERFACES(CIScopePlugin)
public:
    CTektronixTDS1012BPlugin();
};

#endif // TEKTRONIXTDS1012BPLUGIN_H
