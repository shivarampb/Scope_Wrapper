// TektronixTDS1012BPlugin.cpp
#include "TektronixTDS1012BPlugin.h"
#include "ScopeFamilies.h"

CTektronixTDS1012BPlugin::CTektronixTDS1012BPlugin()
{
    m_info = makeScopePluginInfo(
        "Tektronix TDS1012B", "1.0.0", "Tektronix", "TDS1012B",
        "Tektronix TDS1012B 100 MHz 2-Channel Oscilloscope",
        QStringList() << "USB");

    m_caps = makeScopeCaps(2, 100e6, 1e9, 2500, 5e-9, 50.0, 2e-3, 5.0, true);

    m_dialect = tektronixDialect();
}
