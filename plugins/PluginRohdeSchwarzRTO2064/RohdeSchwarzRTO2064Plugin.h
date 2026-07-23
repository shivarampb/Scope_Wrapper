// RohdeSchwarzRTO2064Plugin.h — auto-structured per-model scope plugin (derives CRohdeSchwarzScopeBase).
#ifndef ROHDESCHWARZRTO2064PLUGIN_H
#define ROHDESCHWARZRTO2064PLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CRohdeSchwarzRTO2064Plugin : public QObject, public CRohdeSchwarzScopeBase {
    Q_OBJECT
#ifndef SCOPE_NO_PLUGIN_METADATA
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
#endif
    Q_INTERFACES(CIScopePlugin)
public:
    CRohdeSchwarzRTO2064Plugin();
};

#endif // ROHDESCHWARZRTO2064PLUGIN_H
