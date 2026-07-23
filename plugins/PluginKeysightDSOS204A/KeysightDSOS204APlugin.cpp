// KeysightDSOS204APlugin.cpp
#include "KeysightDSOS204APlugin.h"
#include "ScopeFamilies.h"

CKeysightDSOS204APlugin::CKeysightDSOS204APlugin()
{
    m_info = makeScopePluginInfo(
        "Keysight DSOS204A", "1.0.0", "Keysight", "DSOS204A",
        "Keysight Infiniium DSOS204A 2 GHz 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 2e9, 20e9, 100000000, 5e-10, 20.0, 1e-3, 5.0, false);

    m_dialect = keysightInfiniiumDialect();
}
