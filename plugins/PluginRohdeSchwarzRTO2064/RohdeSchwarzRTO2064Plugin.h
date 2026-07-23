// RohdeSchwarzRTO2064Plugin.h — auto-structured per-model scope plugin (derives CRohdeSchwarzScopeBase).
#ifndef ROHDESCHWARZRTO2064PLUGIN_H
#define ROHDESCHWARZRTO2064PLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CRohdeSchwarzRTO2064Plugin : public QObject, public CRohdeSchwarzScopeBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CRohdeSchwarzRTO2064Plugin();
};

#endif // ROHDESCHWARZRTO2064PLUGIN_H
