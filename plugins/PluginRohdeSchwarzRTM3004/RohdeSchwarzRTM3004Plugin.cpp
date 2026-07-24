// RohdeSchwarzRTM3004Plugin.cpp
#include "RohdeSchwarzRTM3004Plugin.h"
#include "ScopeFamilies.h"

CRohdeSchwarzRTM3004Plugin::CRohdeSchwarzRTM3004Plugin()
{
    m_info = makeScopePluginInfo(
        "RohdeSchwarz RTM3004", "1.0.0", "Rohde & Schwarz", "RTM3004",
        "Rohde & Schwarz RTM3004 1 GHz 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 1e9, 5e9, 40000000, 1e-9, 500.0, 1e-3, 10.0, true);

    m_dialect = rohdeSchwarzDialect();
}
