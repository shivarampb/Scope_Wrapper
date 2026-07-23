// TektronixMDO34Plugin.h — auto-structured per-model scope plugin (derives CTektronixScopeBase).
#ifndef TEKTRONIXMDO34PLUGIN_H
#define TEKTRONIXMDO34PLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CTektronixMDO34Plugin : public QObject, public CTektronixScopeBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CTektronixMDO34Plugin();
};

#endif // TEKTRONIXMDO34PLUGIN_H
