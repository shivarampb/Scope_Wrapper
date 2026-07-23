// TektronixTDS2024CPlugin.h — auto-structured per-model scope plugin (derives CTektronixScopeBase).
#ifndef TEKTRONIXTDS2024CPLUGIN_H
#define TEKTRONIXTDS2024CPLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CTektronixTDS2024CPlugin : public QObject, public CTektronixScopeBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CTektronixTDS2024CPlugin();
};

#endif // TEKTRONIXTDS2024CPLUGIN_H
