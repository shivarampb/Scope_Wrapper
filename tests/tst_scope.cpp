// tst_scope.cpp — Qt Test suite for ScopeCore + representative plugins.
//
// Unit tests (no I/O): command formatting, parameter validation, error strings,
// VISA resource strings, plugin version compatibility.
// Integration tests: drive real plugin code through the VISA simulator
// (tests/visastub) and assert the generated SCPI commands and the decoded
// waveform for each vendor dialect — no hardware required.

#include <QtTest/QtTest>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "CScpiCommandBuilder.h"
#include "ScopeError.h"
#include "ScopeTypes.h"
#include "VisaHelper.h"
#include "ScopeManager.h"
#include "ScopeFamilies.h"

#include "KeysightDSOX2012APlugin.h"
#include "KeysightMSO6054APlugin.h"
#include "TektronixMDO34Plugin.h"
#include "RohdeSchwarzRTM3004Plugin.h"
#include "LeCroyWaveSurfer42XSPlugin.h"

#include "visastub_api.h"

// Read commands[<key>].template from a models/<model>/scpi_map.json file.
static QString mapTemplate(const QString& model, const QString& key)
{
    QFile f(QStringLiteral(SCOPE_MODELS_DIR) + "/" + model + "/scpi_map.json");
    if (!f.open(QIODevice::ReadOnly)) return QString();
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    return root.value("commands").toObject().value(key).toObject().value("template").toString();
}

static S_ConnectionConfig lanConfig()
{
    S_ConnectionConfig c;
    c.m_enumProtocol = Enum_CommunicationProtocol::TCPIP;
    strncpy(c.m_szIpAddress, "127.0.0.1", sizeof(c.m_szIpAddress) - 1);
    return c;
}

class TstScope : public QObject {
    Q_OBJECT
private slots:
    void init() { visastub::reset(); }

    // ---------------- Unit tests ----------------

    void formatValue()
    {
        QCOMPARE(CScpiCommandBuilder::formatValue(0.5), QString("0.5"));
        QCOMPARE(CScpiCommandBuilder::formatValue(1000.0), QString("1000"));
        QVERIFY(CScpiCommandBuilder::formatValue(2e-8).contains("e-08") ||
                CScpiCommandBuilder::formatValue(2e-8).contains("2e-08"));
    }

    void validation()
    {
        S_ScopeCapabilities caps = makeScopeCaps(2, 100e6, 2e9, 100000, 5e-9, 50.0, 1e-3, 5.0, false);
        QVERIFY(CScpiCommandBuilder::validateChannel(caps, 1).isSuccess());
        QCOMPARE(CScpiCommandBuilder::validateChannel(caps, 3).code(), ScopeErrorCode::INVALID_CHANNEL);
        QCOMPARE(CScpiCommandBuilder::validateVerticalScale(caps, 1, 100.0).code(),
                 ScopeErrorCode::INVALID_VOLTS_PER_DIV);
        QVERIFY(CScpiCommandBuilder::validateVerticalScale(caps, 1, 0.5).isSuccess());
        QCOMPARE(CScpiCommandBuilder::validateTimebaseScale(caps, 1e3).code(),
                 ScopeErrorCode::INVALID_SECONDS_PER_DIV);
    }

    void errorStrings()
    {
        ScopeError e(ScopeErrorCode::NO_TRIGGER);
        QVERIFY(e.toString().contains("No trigger"));
        QVERIFY(!ScopeError(ScopeErrorCode::SUCCESS).toString().isEmpty());
        QVERIFY(ScopeError().isSuccess());
    }

    void visaResourceStrings()
    {
        S_ConnectionConfig tcp; tcp.m_enumProtocol = Enum_CommunicationProtocol::TCPIP;
        strncpy(tcp.m_szIpAddress, "192.168.0.10", sizeof(tcp.m_szIpAddress) - 1);
        QCOMPARE(tcp.toVisaResourceString(), QString("TCPIP0::192.168.0.10::inst0::INSTR"));

        S_ConnectionConfig sock; sock.m_enumProtocol = Enum_CommunicationProtocol::TCPIP;
        strncpy(sock.m_szIpAddress, "10.0.0.5", sizeof(sock.m_szIpAddress) - 1);
        sock.m_u32Port = 5000;
        QCOMPARE(sock.toVisaResourceString(), QString("TCPIP0::10.0.0.5::5000::SOCKET"));

        S_ConnectionConfig gpib; gpib.m_enumProtocol = Enum_CommunicationProtocol::GPIB;
        gpib.m_u32GpibAddress = 7;
        QCOMPARE(gpib.toVisaResourceString(), QString("GPIB0::7::INSTR"));

        S_ConnectionConfig usb; usb.m_enumProtocol = Enum_CommunicationProtocol::USB;
        strncpy(usb.m_szUsbVendorId,  "0x0957", sizeof(usb.m_szUsbVendorId) - 1);
        strncpy(usb.m_szUsbProductId, "0x1798", sizeof(usb.m_szUsbProductId) - 1);
        QCOMPARE(usb.toVisaResourceString(), QString("USB0::0x0957::0x1798::INSTR"));
    }

    void versionCompatibility()
    {
        S_PluginInfo info = makeScopePluginInfo("X", "1.0.0", "V", "M", "d", QStringList() << "USB");
        QVERIFY(info.isCompatible("1.0.0"));
        QVERIFY(info.isCompatible("1.5.0"));
        QVERIFY(!info.isCompatible("0.9.0"));
        QVERIFY(!info.isCompatible("2.1.0"));
    }

    void managerRejectsUnknownPlugin()
    {
        CScopeManager mgr;
        QVERIFY(mgr.getAvailablePlugins().isEmpty());
        QCOMPARE(mgr.createInstance(1, "NoSuchPlugin").code(), ScopeErrorCode::PLUGIN_NOT_FOUND);
        QCOMPARE(mgr.createInstance(0, "x").code(), ScopeErrorCode::INVALID_SCOPE_NUMBER);
    }

    // ---------------- Integration tests (via VISA simulator) ----------------

    void keysightRoundTrip()
    {
        CKeysightDSOX2012APlugin scope;
        CIScopePlugin& s = scope; // drive through the interface (as the manager does)
        QCOMPARE(s.getCapabilities().m_u32NumberOfChannels, static_cast<U32BIT>(2));
        QVERIFY(s.connect(1, lanConfig()).isSuccess());

        QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
        QVERIFY(visastub::sawCommand(":CHANnel1:SCALe 0.5"));

        QVERIFY(s.setTimebaseScale(1, 1e-6).isSuccess());
        QVERIFY(visastub::sawCommand(":TIMebase:SCALe 1e-06"));

        S_TriggerConfig trig; trig.m_u32Source = 1; trig.m_dLevel = 1.5;
        QVERIFY(s.setTrigger(1, trig).isSuccess());
        QVERIFY(visastub::sawCommand(":TRIGger:EDGE:SOURce CHANnel1"));
        QVERIFY(visastub::sawCommand(":TRIGger:EDGE:LEVel 1.5"));

        double vpp = 0.0;
        QVERIFY(s.measure(1, 1, Enum_MeasurementType::Vpp, vpp).isSuccess());
        QCOMPARE(vpp, 2.5);

        S_Waveform wf;
        QVERIFY(s.captureWaveform(1, 1, wf).isSuccess());
        QCOMPARE(wf.m_dVolts.size(), 10);
        QVERIFY(qAbs(wf.m_dVolts.at(0) - 0.0) < 1e-6);
        QVERIFY(qAbs(wf.m_dVolts.at(5) - 0.5) < 1e-6);
    }

    void tektronixRoundTrip()
    {
        CTektronixMDO34Plugin scope;
        CIScopePlugin& s = scope;
        QVERIFY(s.connect(1, lanConfig()).isSuccess());
        QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
        QVERIFY(visastub::sawCommand("CH1:SCAle 0.5"));

        double val = 0.0;
        QVERIFY(s.measure(1, 1, Enum_MeasurementType::Vpp, val).isSuccess());
        QCOMPARE(val, 1.0);

        S_Waveform wf;
        QVERIFY(s.captureWaveform(1, 1, wf).isSuccess());
        QCOMPARE(wf.m_dVolts.size(), 10);
        QVERIFY(qAbs(wf.m_dVolts.at(5) - 0.5) < 1e-6);
    }

    void rohdeSchwarzRoundTrip()
    {
        CRohdeSchwarzRTM3004Plugin scope;
        CIScopePlugin& s = scope;
        QVERIFY(s.connect(1, lanConfig()).isSuccess());
        QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
        QVERIFY(visastub::sawCommand("CHANnel1:SCALe 0.5"));

        S_Waveform wf;
        QVERIFY(s.captureWaveform(1, 1, wf).isSuccess());
        QCOMPARE(wf.m_dVolts.size(), 10);
        QVERIFY(qAbs(wf.m_dVolts.at(9) - 0.9) < 1e-5);
    }

    void leCroyRoundTrip()
    {
        CLeCroyWaveSurfer42XSPlugin scope;
        CIScopePlugin& s = scope;
        QVERIFY(s.connect(1, lanConfig()).isSuccess());
        QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
        QVERIFY(visastub::sawCommand("C1:VDIV 0.5"));

        double pkpk = 0.0;
        QVERIFY(s.measure(1, 1, Enum_MeasurementType::Vpp, pkpk).isSuccess());
        QCOMPARE(pkpk, 1.0);

        S_Waveform wf;
        QVERIFY(s.captureWaveform(1, 1, wf).isSuccess());
        QCOMPARE(wf.m_dVolts.size(), 10);
        QVERIFY(qAbs(wf.m_dVolts.at(5) - 0.5) < 1e-6);
    }

    // ---- Extended features: memory/sample/save-recall/screenshot ----

    void extendedFeatures()
    {
        CKeysightDSOX2012APlugin scope;
        CIScopePlugin& s = scope;
        QVERIFY(s.connect(1, lanConfig()).isSuccess());

        QVERIFY(s.setMemoryDepth(1, 100000).isSuccess());
        QVERIFY(visastub::sawCommand(":ACQuire:POINts 100000"));
        // Beyond model max memory (100k) -> out of range.
        QCOMPARE(s.setMemoryDepth(1, 10000000).code(), ScopeErrorCode::PARAMETER_OUT_OF_RANGE);
        // Sample rate is derived on InfiniiVision -> not supported.
        QCOMPARE(s.setSampleRate(1, 1e9).code(), ScopeErrorCode::NOT_SUPPORTED);

        QVERIFY(s.saveSetup(1, 3).isSuccess());
        QVERIFY(visastub::sawCommand("*SAV 3"));
        QVERIFY(s.recallSetup(1, 3).isSuccess());
        QVERIFY(visastub::sawCommand("*RCL 3"));

        QByteArray img;
        QVERIFY(s.getScreenshot(1, img).isSuccess());
        QVERIFY(visastub::sawCommand(":DISPlay:DATA? PNG,COLor"));
        QVERIFY(!img.isEmpty());
        QCOMPARE(img.mid(1, 3), QByteArray("PNG"));
    }

    void msoDigitalChannels()
    {
        CKeysightMSO6054APlugin mso;
        CIScopePlugin& s = mso;
        QVERIFY(s.getCapabilities().m_bIsMSO);
        QVERIFY(s.connect(1, lanConfig()).isSuccess());

        QVERIFY(s.setDigitalChannelEnable(1, 0, true).isSuccess());
        QVERIFY(visastub::sawCommand(":DIGital0:DISPlay ON"));
        QVERIFY(s.setDigitalThreshold(1, 1, 1.4).isSuccess());
        QVERIFY(visastub::sawCommand(":POD1:THReshold 1.4"));
        // Out-of-range digital channel (model has 16: 0..15).
        QCOMPARE(s.setDigitalChannelEnable(1, 99, true).code(), ScopeErrorCode::INVALID_CHANNEL);
    }

    void nonMsoRejectsDigital()
    {
        CKeysightDSOX2012APlugin scope; // not an MSO
        CIScopePlugin& s = scope;
        QVERIFY(s.connect(1, lanConfig()).isSuccess());
        QCOMPARE(s.setDigitalChannelEnable(1, 0, true).code(), ScopeErrorCode::NOT_SUPPORTED);
    }

    // ---- SCPI map <-> code cross-check ----

    void scpiMapMatchesCode()
    {
        // Keysight: map "%1"=channel number -> ":CHANnel1:SCALe 0.5"
        {
            QString t = mapTemplate("DSOX2012A", "verticalScale");
            QVERIFY2(!t.isEmpty(), "missing DSOX2012A verticalScale template");
            const QString expected = t.arg(1).arg("0.5");
            CKeysightDSOX2012APlugin scope; CIScopePlugin& s = scope;
            QVERIFY(s.connect(1, lanConfig()).isSuccess());
            QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
            QVERIFY2(visastub::sawCommand(expected.toStdString()), qPrintable("expected: " + expected));
        }
        // Tektronix: map "%1"=source (CH1) -> "CH1:SCAle 0.5"
        {
            QString t = mapTemplate("MDO34", "verticalScale");
            QVERIFY2(!t.isEmpty(), "missing MDO34 verticalScale template");
            const QString expected = t.arg("CH1").arg("0.5");
            CTektronixMDO34Plugin scope; CIScopePlugin& s = scope;
            QVERIFY(s.connect(1, lanConfig()).isSuccess());
            QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
            QVERIFY2(visastub::sawCommand(expected.toStdString()), qPrintable("expected: " + expected));
        }
        // LeCroy: map "%1"=source (C1) -> "C1:VDIV 0.5"
        {
            QString t = mapTemplate("WaveSurfer42XS", "verticalScale");
            QVERIFY2(!t.isEmpty(), "missing WaveSurfer42XS verticalScale template");
            const QString expected = t.arg("C1").arg("0.5");
            CLeCroyWaveSurfer42XSPlugin scope; CIScopePlugin& s = scope;
            QVERIFY(s.connect(1, lanConfig()).isSuccess());
            QVERIFY(s.setVerticalScale(1, 1, 0.5).isSuccess());
            QVERIFY2(visastub::sawCommand(expected.toStdString()), qPrintable("expected: " + expected));
        }
    }

};

QTEST_MAIN(TstScope)
#include "tst_scope.moc"
