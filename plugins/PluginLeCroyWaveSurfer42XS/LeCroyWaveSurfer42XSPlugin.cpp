// LeCroyWaveSurfer42XSPlugin.cpp
#include "LeCroyWaveSurfer42XSPlugin.h"
#include "ScopeFamilies.h"

CLeCroyWaveSurfer42XSPlugin::CLeCroyWaveSurfer42XSPlugin()
{
    m_info = makeScopePluginInfo(
        "LeCroy WaveSurfer42XS", "1.0.0", "LeCroy", "WaveSurfer 42XS",
        "Teledyne LeCroy WaveSurfer 42XS 400 MHz 4-Channel Oscilloscope",
        QStringList() << "USB" << "TCPIP");

    m_caps = makeScopeCaps(4, 400e6, 5e9, 25000000, 2e-10, 1000.0, 2e-3, 10.0, true);

    m_dialect = leCroyDialect();
}
