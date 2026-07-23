// KeysightMSO6054APlugin.cpp
#include "KeysightMSO6054APlugin.h"
#include "ScopeFamilies.h"

CKeysightMSO6054APlugin::CKeysightMSO6054APlugin()
{
    m_info = makeScopePluginInfo(
        "Keysight MSO6054A", "1.0.0", "Agilent", "MSO6054A",
        "Agilent InfiniiVision MSO6054A 500 MHz 4-Channel Mixed-Signal Oscilloscope",
        QStringList() << "USB" << "TCPIP" << "LXI");

    m_caps = makeScopeCaps(4, 500e6, 4e9, 1000000, 5e-9, 50.0, 1e-3, 5.0, false, true, 16);

    m_dialect = keysightInfiniiVisionDialect();
}
