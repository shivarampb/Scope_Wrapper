// KeysightDSOX2012APlugin.h
//
// Keysight/Agilent InfiniiVision DSOX2012A (100 MHz, 2 ch). Derives the shared
// CVisaScopePlugin base and only supplies identity, capabilities, and the
// InfiniiVision dialect. Structural analogue of the ELoad_R2 per-model plugin
// (QObject + interface + Q_PLUGIN_METADATA), now with transport factored out.

#ifndef KEYSIGHTDSOX2012APLUGIN_H
#define KEYSIGHTDSOX2012APLUGIN_H

#include <QObject>
#include "CVisaScopePlugin.h"

class CKeysightDSOX2012APlugin : public QObject, public CVisaScopePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)
public:
    CKeysightDSOX2012APlugin();
};

#endif // KEYSIGHTDSOX2012APLUGIN_H
