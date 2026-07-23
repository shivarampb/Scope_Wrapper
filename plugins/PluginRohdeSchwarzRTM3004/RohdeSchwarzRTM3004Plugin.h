// RohdeSchwarzRTM3004Plugin.h — auto-structured per-model scope plugin (derives CRohdeSchwarzScopeBase).
#ifndef ROHDESCHWARZRTM3004PLUGIN_H
#define ROHDESCHWARZRTM3004PLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CRohdeSchwarzRTM3004Plugin : public QObject, public CRohdeSchwarzScopeBase {
    Q_OBJECT
#ifndef SCOPE_NO_PLUGIN_METADATA
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
#endif
    Q_INTERFACES(CIScopePlugin)
public:
    CRohdeSchwarzRTM3004Plugin();
};

#endif // ROHDESCHWARZRTM3004PLUGIN_H
