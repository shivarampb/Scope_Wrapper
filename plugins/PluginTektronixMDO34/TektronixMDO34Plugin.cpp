// TektronixMDO34Plugin.cpp
#include "TektronixMDO34Plugin.h"
#include "ScopeFamilies.h"

CTektronixMDO34Plugin::CTektronixMDO34Plugin()
{
    m_info = makeScopePluginInfo(
        "Tektronix MDO34", "1.0.0", "Tektronix", "MDO34",
        "Tektronix 3 Series MDO34 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 1e9, 5e9, 10000000, 4e-10, 1000.0, 1e-3, 10.0, true);

    m_dialect = tektronixDialect();
}
