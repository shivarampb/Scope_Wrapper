// tst_manager_dl.cpp — dynamic-load test for CScopeManager.
//
// Loads the real plugin .so files (built with Q_PLUGIN_METADATA enabled) via
// QPluginLoader and drives one through the manager against the VISA simulator.
// This covers the production QPluginLoader path that the direct-instantiation
// unit suite (tst_scope) bypasses.

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QString>
#include <cstdlib>

#include "ScopeManager.h"
#include "ScopeTypes.h"
#include "visastub_api.h"

static QString pluginDir()
{
    if (const char* env = std::getenv("SCOPE_PLUGIN_DIR"))
        return QString::fromLocal8Bit(env);
    return QCoreApplication::applicationDirPath() + "/bin/plugins";
}

class TstManagerDl : public QObject {
    Q_OBJECT
private slots:
    void init() { visastub::reset(); }

    void loadsAllBuiltPlugins()
    {
        CScopeManager mgr;
        mgr.loadPlugins(pluginDir());
        const QStringList names = mgr.getAvailablePlugins();
        QVERIFY2(!names.isEmpty(), "no plugins loaded from SCOPE_PLUGIN_DIR");
        QVERIFY(names.contains("Keysight DSOX2012A"));
        QVERIFY(names.contains("Tektronix MDO34"));
        QVERIFY(names.contains("LeCroy WaveSurfer42XS"));
    }

    void driveThroughManager()
    {
        CScopeManager mgr;
        mgr.loadPlugins(pluginDir());

        QCOMPARE(mgr.createInstance(1, "Keysight DSOX2012A").code(), ScopeErrorCode::SUCCESS);

        S_ConnectionConfig cfg;
        cfg.m_enumProtocol = Enum_CommunicationProtocol::TCPIP;
        strncpy(cfg.m_szIpAddress, "127.0.0.1", sizeof(cfg.m_szIpAddress) - 1);
        QVERIFY(mgr.connect(1, cfg).isSuccess());
        QVERIFY(mgr.isConnected(1));

        QVERIFY(mgr.setVerticalScale(1, 1, 0.5).isSuccess());
        QVERIFY(visastub::sawCommand(":CHANnel1:SCALe 0.5"));

        double vpp = 0.0;
        QVERIFY(mgr.measure(1, 1, Enum_MeasurementType::Vpp, vpp).isSuccess());
        QCOMPARE(vpp, 2.5);

        S_Waveform wf;
        QVERIFY(mgr.captureWaveform(1, 1, wf).isSuccess());
        QCOMPARE(wf.m_dVolts.size(), 10);
        QVERIFY(qAbs(wf.m_dVolts.at(5) - 0.5) < 1e-6);

        QVERIFY(mgr.disconnect(1).isSuccess());
    }

    void managerFailurePaths()
    {
        CScopeManager mgr;
        mgr.loadPlugins(pluginDir());

        QCOMPARE(mgr.createInstance(2, "NoSuchPlugin").code(), ScopeErrorCode::PLUGIN_NOT_FOUND);
        QCOMPARE(mgr.createInstance(1, "Tektronix MDO34").code(), ScopeErrorCode::SUCCESS);
        QCOMPARE(mgr.createInstance(1, "Tektronix MDO34").code(), ScopeErrorCode::ALREADY_CONNECTED);
        // Operating on an instance that was never created.
        QCOMPARE(mgr.reset(99).code(), ScopeErrorCode::INVALID_SCOPE_NUMBER);
    }

    void autoSelectFromIdn()
    {
        CScopeManager mgr;
        mgr.loadPlugins(pluginDir());

        // Realistic Keysight IDN uses "DSO-X 2012A"; the plugin model is
        // "DSOX2012A". Normalized matching must still resolve it.
        const QString idn = "KEYSIGHT TECHNOLOGIES,DSO-X 2012A,MY51330623,07.30";
        QCOMPARE(mgr.matchPluginForModel(idn), QString("Keysight DSOX2012A"));
        QCOMPARE(mgr.createInstanceFromIdn(1, idn).code(), ScopeErrorCode::SUCCESS);
        QCOMPARE(mgr.getInstancePlugin(1), QString("Keysight DSOX2012A"));

        QVERIFY(mgr.matchPluginForModel("VENDOR,UNKNOWN-9999,x,y").isEmpty());
        QCOMPARE(mgr.createInstanceFromIdn(2, "no match here").code(), ScopeErrorCode::PLUGIN_NOT_FOUND);
    }
};

QTEST_MAIN(TstManagerDl)
#include "tst_manager_dl.moc"
