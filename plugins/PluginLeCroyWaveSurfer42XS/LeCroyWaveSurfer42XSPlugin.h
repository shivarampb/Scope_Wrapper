// LeCroyWaveSurfer42XSPlugin.h — auto-structured per-model scope plugin (derives CLeCroyScopeBase).
#ifndef LECROYWAVESURFER42XSPLUGIN_H
#define LECROYWAVESURFER42XSPLUGIN_H

#include <QObject>
#include "ScopeFamilies.h"

class CLeCroyWaveSurfer42XSPlugin : public QObject, public CLeCroyScopeBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CLeCroyWaveSurfer42XSPlugin();
};

#endif // LECROYWAVESURFER42XSPLUGIN_H
