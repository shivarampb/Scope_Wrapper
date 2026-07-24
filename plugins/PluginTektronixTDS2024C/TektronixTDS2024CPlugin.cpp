// TektronixTDS2024CPlugin.cpp
#include "TektronixTDS2024CPlugin.h"
#include "ScopeFamilies.h"

CTektronixTDS2024CPlugin::CTektronixTDS2024CPlugin()
{
    m_info = makeScopePluginInfo(
        "Tektronix TDS2024C", "1.0.0", "Tektronix", "TDS2024C",
        "Tektronix TDS2024C 200 MHz 4-Channel Oscilloscope",
        QStringList() << "USB");

    m_caps = makeScopeCaps(4, 200e6, 2e9, 2500, 2.5e-9, 50.0, 2e-3, 5.0, true);

    m_dialect = tektronixDialect();
}
