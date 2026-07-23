// RohdeSchwarzRTO2064Plugin.cpp
#include "RohdeSchwarzRTO2064Plugin.h"
#include "ScopeFamilies.h"

CRohdeSchwarzRTO2064Plugin::CRohdeSchwarzRTO2064Plugin()
{
    m_info = makeScopePluginInfo(
        "RohdeSchwarz RTO2064", "1.0.0", "Rohde & Schwarz", "RTO2064",
        "Rohde & Schwarz RTO2064 6 GHz 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 6e9, 20e9, 200000000, 25e-12, 10000.0, 1e-3, 10.0, true);

    m_dialect = rohdeSchwarzDialect();
}
