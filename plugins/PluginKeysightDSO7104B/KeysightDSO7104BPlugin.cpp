// KeysightDSO7104BPlugin.cpp
#include "KeysightDSO7104BPlugin.h"
#include "ScopeFamilies.h"

CKeysightDSO7104BPlugin::CKeysightDSO7104BPlugin()
{
    m_info = makeScopePluginInfo(
        "Keysight DSO7104B", "1.0.0", "Agilent", "DSO7104B",
        "Agilent InfiniiVision DSO7104B 1 GHz 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 1e9, 4e9, 1000000, 2e-9, 50.0, 1e-3, 5.0, false);

    m_dialect = keysightInfiniiVisionDialect();
}
