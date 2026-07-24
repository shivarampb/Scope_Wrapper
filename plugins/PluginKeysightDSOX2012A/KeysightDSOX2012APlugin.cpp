// KeysightDSOX2012APlugin.cpp
#include "KeysightDSOX2012APlugin.h"
#include "ScopeFamilies.h"

CKeysightDSOX2012APlugin::CKeysightDSOX2012APlugin()
{
    m_info = makeScopePluginInfo(
        "Keysight DSOX2012A", "1.0.0", "Keysight/Agilent", "DSOX2012A",
        "Keysight InfiniiVision DSOX2012A 100 MHz 2-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    // channels, bandwidth, maxSR, memDepth, min/max s/div, min/max V/div, hasGND
    m_caps = makeScopeCaps(2, 100e6, 2e9, 100000, 5e-9, 50.0, 1e-3, 5.0, /*hasGnd*/false);

    m_dialect = keysightInfiniiVisionDialect();
}
