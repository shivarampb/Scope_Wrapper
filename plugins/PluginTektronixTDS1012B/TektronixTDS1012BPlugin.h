// TektronixTDS1012BPlugin.h — auto-structured per-model scope plugin (derives CTektronixScopeBase).
#ifndef TEKTRONIXTDS1012BPLUGIN_H
#define TEKTRONIXTDS1012BPLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CTektronixTDS1012BPlugin : public QObject, public CTektronixScopeBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CTektronixTDS1012BPlugin();
};

#endif // TEKTRONIXTDS1012BPLUGIN_H
