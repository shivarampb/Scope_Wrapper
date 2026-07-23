// KeysightDSOX2012APlugin.cpp
//
// Implements the DSOX2012A plugin. Follows the ELoad_R2 plugin structure:
// VISA connect (viOpenDefaultRM -> viOpen -> viSetAttribute -> *IDN?),
// a shared sendCommand/sendQuery, buildCommand for any per-model prefixing,
// visaErrorToScope, validateResponse, and the SCPI methods themselves.
//
// SCPI reference: Keysight InfiniiVision 2000 X-Series Oscilloscopes
// Programmer's Guide. Section citations are noted at the relevant commands,
// mirroring the reference's "// ZUP Manual Section x.y" discipline.

#include "KeysightDSOX2012APlugin.h"
#include "VisaHelper.h"
#include "CScpiCommandBuilder.h"
#include <QStringList>
#include <QDebug>

CKeysightDSOX2012APlugin::CKeysightDSOX2012APlugin()
{
}

CKeysightDSOX2012APlugin::~CKeysightDSOX2012APlugin()
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        if (it.value().m_vi != VI_NULL)        viClose(it.value().m_vi);
        if (it.value().m_defaultRM != VI_NULL) viClose(it.value().m_defaultRM);
    }
    m_devices.clear();
}

// ============================ Plugin information ============================

S_PluginInfo CKeysightDSOX2012APlugin::getPluginInfo() const
{
    S_PluginInfo s_Info;
    strncpy(s_Info.m_szName,          "Keysight DSOX2012A", PLUGIN_INFO_NAME_SIZE - 1);
    strncpy(s_Info.m_szVersion,       "1.0.0",              PLUGIN_INFO_VERSION_SIZE - 1);
    strncpy(s_Info.m_szManufacturer,  "Keysight/Agilent",   PLUGIN_INFO_MANUFACTURER_SIZE - 1);
    strncpy(s_Info.m_szModelName,     "DSOX2012A",          PLUGIN_INFO_MODEL_NAME_SIZE - 1);
    strncpy(s_Info.m_szDescription,   "Keysight InfiniiVision DSOX2012A 100 MHz 2-Channel Oscilloscope",
                                       PLUGIN_INFO_DESCRIPTION_SIZE - 1);
    strncpy(s_Info.m_szMinCoreVersion,"1.0.0",              PLUGIN_INFO_MIN_CORE_VERSION - 1);
    strncpy(s_Info.m_szMaxCoreVersion,"2.0.0",              PLUGIN_INFO_MAX_CORE_VERSION - 1);

    s_Info.m_StrlstSupportedProtocols << "USB" << "TCPIP" << "LXI";
    return s_Info;
}

S_ScopeCapabilities CKeysightDSOX2012APlugin::getCapabilities() const
{
    S_ScopeCapabilities s_Caps;
    s_Caps.m_u32NumberOfChannels = 2;
    s_Caps.m_u32DigitalChannels  = 0;
    s_Caps.m_dBandwidthHz        = 100e6;    // 100 MHz
    s_Caps.m_dMaxSampleRate      = 2e9;      // 2 GSa/s (half-channel)
    s_Caps.m_u64MaxMemoryDepth   = 100000;   // 100 kpts (standard)
    s_Caps.m_dMinSecondsPerDiv   = 5e-9;     // 5 ns/div
    s_Caps.m_dMaxSecondsPerDiv   = 50.0;     // 50 s/div
    s_Caps.m_bHasFFT             = true;
    s_Caps.m_bIsMSO              = false;

    s_Caps.QlistTriggerTypes << Enum_TriggerType::Edge << Enum_TriggerType::Pulse;
    s_Caps.QlistAcquisitionModes << Enum_AcquisitionMode::Sample
                                 << Enum_AcquisitionMode::PeakDetect
                                 << Enum_AcquisitionMode::Average
                                 << Enum_AcquisitionMode::HighResolution;

    for (U32BIT ch = 1; ch <= s_Caps.m_u32NumberOfChannels; ++ch)
    {
        S_ChannelCapabilities s_Ch;
        s_Ch.m_u32ChannelNumber     = ch;
        s_Ch.m_dMinVoltsPerDiv      = 1e-3;   // 1 mV/div
        s_Ch.m_dMaxVoltsPerDiv      = 5.0;    // 5 V/div (1:1 probe)
        s_Ch.m_dMaxOffset           = 20.0;
        s_Ch.m_dMaxProbeAttenuation = 1000.0;
        s_Ch.m_bHasBandwidthLimit   = true;
        s_Ch.m_bHasACCoupling       = true;
        s_Ch.m_bHasGNDCoupling      = false;  // InfiniiVision: AC/DC only
        s_Ch.m_bIsDigital           = false;
        s_Caps.QlistChannels.append(s_Ch);
    }
    return s_Caps;
}

// ============================ Transport helpers ============================

QString CKeysightDSOX2012APlugin::channelSource(U32BIT in_u32Channel) const
{
    return QString("CHANnel%1").arg(in_u32Channel);
}

QString CKeysightDSOX2012APlugin::buildCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd) const
{
    // InfiniiVision has no daisy-chain/address prefixing, so the command is
    // used verbatim. The hook exists to mirror the reference and to allow
    // model quirks (e.g. header on/off) without touching call sites.
    Q_UNUSED(in_u32ScopeNumber);
    return in_kqstrCmd;
}

ScopeError CKeysightDSOX2012APlugin::visaErrorToScope(ViStatus in_ViStatus, const QString& in_kstrContext) const
{
    if (in_ViStatus == VI_SUCCESS)
    {
        return ScopeError(ScopeErrorCode::SUCCESS);
    }
    const QString kPrefix = in_kstrContext.isEmpty() ? "" : in_kstrContext + ": ";

    switch (in_ViStatus)
    {
    case VI_ERROR_RSRC_NFOUND:
        return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Resource not found");
    case VI_ERROR_TMO:
        return ScopeError(ScopeErrorCode::COMMUNICATION_TIMEOUT, kPrefix + "Timeout");
    case VI_ERROR_CONN_LOST:
        return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Connection lost");
    case VI_ERROR_INV_RSRC_NAME:
        return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Invalid resource name");
    default:
        return ScopeError(ScopeErrorCode::COMMUNICATION_ERROR,
                          kPrefix + QString("VISA error: 0x%1").arg(in_ViStatus, 0, 16));
    }
}

ScopeError CKeysightDSOX2012APlugin::sendCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd)
{
    if (!isConnected(in_u32ScopeNumber))
    {
        return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    }

    S_DeviceInstance& s_Device = m_devices[in_u32ScopeNumber - 1];
    QString qstrCmd = buildCommand(in_u32ScopeNumber, in_kqstrCmd);
    if (!qstrCmd.endsWith('\n')) qstrCmd += '\n';

    QByteArray baCmd = qstrCmd.toLatin1();
    ViUInt32 viRetCount = 0;
    ViStatus viStatus = viWrite(s_Device.m_vi, (ViBuf)baCmd.data(), baCmd.length(), &viRetCount);
    if (viStatus < VI_SUCCESS)
    {
        return visaErrorToScope(viStatus, "Failed to write command");
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CKeysightDSOX2012APlugin::sendQuery(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd, QString& out_qstrResponse)
{
    if (!isConnected(in_u32ScopeNumber))
    {
        return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    }

    S_DeviceInstance& s_Device = m_devices[in_u32ScopeNumber - 1];
    QString qstrCmd = buildCommand(in_u32ScopeNumber, in_kqstrCmd);
    if (!qstrCmd.endsWith('\n')) qstrCmd += '\n';

    QByteArray baCmd = qstrCmd.toLatin1();
    ViUInt32 viRetCount = 0;
    ViStatus viStatus = viWrite(s_Device.m_vi, (ViBuf)baCmd.data(), baCmd.length(), &viRetCount);
    if (viStatus < VI_SUCCESS)
    {
        return visaErrorToScope(viStatus, "Failed to write query");
    }

    char s8arrBuffer[4096];
    viStatus = viRead(s_Device.m_vi, (ViBuf)s8arrBuffer, sizeof(s8arrBuffer) - 1, &viRetCount);
    if (viStatus < VI_SUCCESS && viStatus != VI_ERROR_TMO && viRetCount == 0)
    {
        return visaErrorToScope(viStatus, "Failed to read response");
    }
    s8arrBuffer[viRetCount] = '\0';
    out_qstrResponse = QString::fromLatin1(s8arrBuffer).trimmed();

    if (!validateResponse(out_qstrResponse))
    {
        return ScopeError(ScopeErrorCode::INVALID_RESPONSE, out_qstrResponse);
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// Reads an IEEE 488.2 definite-length block ("#" <ndigits> <length> <bytes>).
ScopeError CKeysightDSOX2012APlugin::readBinaryBlock(U32BIT in_u32ScopeNumber, QByteArray& out_baData)
{
    out_baData.clear();
    S_DeviceInstance& s_Device = m_devices[in_u32ScopeNumber - 1];

    QByteArray baRaw;
    char s8arrChunk[8192];
    ViUInt32 viRetCount = 0;
    ViStatus viStatus = VI_SUCCESS;

    // Read until the driver signals end-of-message (not VI_SUCCESS_MAX_CNT).
    do {
        viStatus = viRead(s_Device.m_vi, (ViBuf)s8arrChunk, sizeof(s8arrChunk), &viRetCount);
        if (viStatus < VI_SUCCESS && viStatus != VI_SUCCESS_MAX_CNT && viRetCount == 0)
        {
            return visaErrorToScope(viStatus, "Failed to read waveform block");
        }
        baRaw.append(s8arrChunk, static_cast<int>(viRetCount));
    } while (viStatus == VI_SUCCESS_MAX_CNT);

    // Parse the block header.
    if (baRaw.isEmpty() || baRaw.at(0) != '#')
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Malformed block header");
    }
    const int iNumDigits = QString(QChar(baRaw.at(1))).toInt();
    if (iNumDigits <= 0)
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Bad block length prefix");
    }
    const int iLen = baRaw.mid(2, iNumDigits).toInt();
    const int iDataStart = 2 + iNumDigits;
    out_baData = baRaw.mid(iDataStart, iLen);
    if (out_baData.size() < iLen)
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Short waveform block");
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

bool CKeysightDSOX2012APlugin::validateResponse(const QString& in_kStrResponse) const
{
    // A well-formed query response is simply non-empty for this instrument.
    return !in_kStrResponse.isEmpty();
}

// ============================ Connection management ============================

ScopeError CKeysightDSOX2012APlugin::connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig)
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (m_devices.contains(key) && m_devices[key].m_bConnected)
    {
        return ScopeError(ScopeErrorCode::ALREADY_CONNECTED);
    }

    S_DeviceInstance s_Device;
    s_Device.m_sConfig = in_sConfig;

    ViStatus viStatus = viOpenDefaultRM(&s_Device.m_defaultRM);
    if (viStatus < VI_SUCCESS)
    {
        return visaErrorToScope(viStatus, "Failed to open resource manager");
    }

    const QString qstrResource = in_sConfig.toVisaResourceString();
    qDebug() << "Connecting to:" << qstrResource;

    viStatus = viOpen(s_Device.m_defaultRM, qstrResource.toLatin1().data(),
                      VI_NULL, VI_NULL, &s_Device.m_vi);
    if (viStatus < VI_SUCCESS)
    {
        viClose(s_Device.m_defaultRM);
        return visaErrorToScope(viStatus, "Failed to open instrument");
    }

    viSetAttribute(s_Device.m_vi, VI_ATTR_TMO_VALUE, in_sConfig.m_u32Timeout);
    if (in_sConfig.m_enumProtocol == Enum_CommunicationProtocol::RS232)
    {
        viSetAttribute(s_Device.m_vi, VI_ATTR_ASRL_BAUD, in_sConfig.m_u32BaudRate);
        viSetAttribute(s_Device.m_vi, VI_ATTR_ASRL_DATA_BITS, 8);
        viSetAttribute(s_Device.m_vi, VI_ATTR_ASRL_PARITY, VI_ASRL_PAR_NONE);
        viSetAttribute(s_Device.m_vi, VI_ATTR_ASRL_STOP_BITS, VI_ASRL_STOP_ONE);
    }

    s_Device.m_bConnected = true;
    m_devices[key] = s_Device;

    // Identify the instrument (*IDN?), reference does the same on connect.
    QString qstrIdn;
    ScopeError err = sendQuery(in_u32ScopeNumber, "*IDN?", qstrIdn);
    if (!err.isSuccess())
    {
        disconnect(in_u32ScopeNumber);
        return ScopeError(ScopeErrorCode::CONNECTION_FAILED, "Device not responding");
    }
    qDebug() << "Device identified as:" << qstrIdn;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CKeysightDSOX2012APlugin::disconnect(U32BIT in_u32ScopeNumber)
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (!m_devices.contains(key))
    {
        return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    }
    S_DeviceInstance& s_Device = m_devices[key];
    if (s_Device.m_vi != VI_NULL)        viClose(s_Device.m_vi);
    if (s_Device.m_defaultRM != VI_NULL) viClose(s_Device.m_defaultRM);
    m_devices.remove(key);
    return ScopeError(ScopeErrorCode::SUCCESS);
}

bool CKeysightDSOX2012APlugin::isConnected(U32BIT in_u32ScopeNumber) const
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (!m_devices.contains(key))
    {
        return false;
    }
    return m_devices[key].m_bConnected && m_devices[key].m_vi != VI_NULL;
}

ScopeError CKeysightDSOX2012APlugin::reset(U32BIT in_u32ScopeNumber)
{
    ScopeError err = sendCommand(in_u32ScopeNumber, "*RST");   // Programmer's Guide: Common Commands
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, "*CLS");
    return err;
}

ScopeError CKeysightDSOX2012APlugin::autoSetup(U32BIT in_u32ScopeNumber)
{
    ScopeError err = sendCommand(in_u32ScopeNumber, ":AUToscale");  // :AUToscale
    if (!err.isSuccess())
    {
        return ScopeError(ScopeErrorCode::AUTOSET_FAILED, err.description());
    }
    return err;
}

ScopeError CKeysightDSOX2012APlugin::checkOperationComplete(U32BIT in_u32ScopeNumber, bool& out_bComplete)
{
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, "*OPC?", qstrResp);   // *OPC?
    out_bComplete = (err.isSuccess() && qstrResp.trimmed() == "1");
    return err;
}

// ============================ Vertical ============================

ScopeError CKeysightDSOX2012APlugin::setChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bEnable)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:DISPlay ON|OFF
    const QString cmd = QString(":%1:DISPlay %2").arg(channelSource(in_u32Channel)).arg(in_bEnable ? "ON" : "OFF");
    return sendCommand(in_u32ScopeNumber, cmd);
}

ScopeError CKeysightDSOX2012APlugin::setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv)
{
    ScopeError v = CScpiCommandBuilder::validateVerticalScale(getCapabilities(), in_u32Channel, in_dVoltsPerDiv);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:SCALe <volts/div>
    const QString cmd = QString(":%1:SCALe %2")
                            .arg(channelSource(in_u32Channel))
                            .arg(CScpiCommandBuilder::formatValue(in_dVoltsPerDiv));
    ScopeError err = sendCommand(in_u32ScopeNumber, cmd);
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_VERTICAL_FAILED, err.description());
}

ScopeError CKeysightDSOX2012APlugin::setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dOffsetVolts)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:OFFSet <volts>
    const QString cmd = QString(":%1:OFFSet %2")
                            .arg(channelSource(in_u32Channel))
                            .arg(CScpiCommandBuilder::formatValue(in_dOffsetVolts));
    return sendCommand(in_u32ScopeNumber, cmd);
}

ScopeError CKeysightDSOX2012APlugin::setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Coupling in_enumCoupling)
{
    ScopeError v = CScpiCommandBuilder::validateCoupling(getCapabilities(), in_u32Channel, in_enumCoupling);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:COUPling AC|DC
    const QString cmd = QString(":%1:COUPling %2")
                            .arg(channelSource(in_u32Channel))
                            .arg(CScpiCommandBuilder::couplingToken(in_enumCoupling));
    return sendCommand(in_u32ScopeNumber, cmd);
}

ScopeError CKeysightDSOX2012APlugin::setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dAttenuation)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:PROBe <ratio>
    const QString cmd = QString(":%1:PROBe %2")
                            .arg(channelSource(in_u32Channel))
                            .arg(CScpiCommandBuilder::formatValue(in_dAttenuation));
    return sendCommand(in_u32ScopeNumber, cmd);
}

ScopeError CKeysightDSOX2012APlugin::setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bLimit)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;
    // :CHANnel<n>:BWLimit ON|OFF
    const QString cmd = QString(":%1:BWLimit %2").arg(channelSource(in_u32Channel)).arg(in_bLimit ? "ON" : "OFF");
    return sendCommand(in_u32ScopeNumber, cmd);
}

// ============================ Horizontal ============================

ScopeError CKeysightDSOX2012APlugin::setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv)
{
    ScopeError v = CScpiCommandBuilder::validateTimebaseScale(getCapabilities(), in_dSecondsPerDiv);
    if (!v.isSuccess()) return v;
    // :TIMebase:SCALe <s/div>
    const QString cmd = QString(":TIMebase:SCALe %1").arg(CScpiCommandBuilder::formatValue(in_dSecondsPerDiv));
    ScopeError err = sendCommand(in_u32ScopeNumber, cmd);
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_TIMEBASE_FAILED, err.description());
}

ScopeError CKeysightDSOX2012APlugin::setHorizontalPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dDelaySeconds)
{
    // :TIMebase:POSition <s>
    const QString cmd = QString(":TIMebase:POSition %1").arg(CScpiCommandBuilder::formatValue(in_dDelaySeconds));
    return sendCommand(in_u32ScopeNumber, cmd);
}

// ============================ Trigger ============================

ScopeError CKeysightDSOX2012APlugin::setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_sTrigger.m_u32Source);
    if (!v.isSuccess()) return ScopeError(ScopeErrorCode::INVALID_SOURCE, v.description());

    // Edge trigger setup (Programmer's Guide: :TRIGger[:EDGE] subsystem).
    ScopeError err = sendCommand(in_u32ScopeNumber, ":TRIGger:MODE EDGE");
    if (err.isSuccess())
        err = sendCommand(in_u32ScopeNumber, QString(":TRIGger:EDGE:SOURce %1").arg(channelSource(in_sTrigger.m_u32Source)));
    if (err.isSuccess())
        err = sendCommand(in_u32ScopeNumber, QString(":TRIGger:EDGE:SLOPe %1").arg(CScpiCommandBuilder::slopeToken(in_sTrigger.m_enumSlope)));
    if (err.isSuccess())
        err = sendCommand(in_u32ScopeNumber, QString(":TRIGger:EDGE:LEVel %1").arg(CScpiCommandBuilder::formatValue(in_sTrigger.m_dLevel)));
    if (err.isSuccess())
        err = setTriggerMode(in_u32ScopeNumber, in_sTrigger.m_enumMode);

    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_TRIGGER_FAILED, err.description());
}

ScopeError CKeysightDSOX2012APlugin::setTriggerLevel(U32BIT in_u32ScopeNumber, FDOUBLE in_dLevelVolts)
{
    const QString cmd = QString(":TRIGger:EDGE:LEVel %1").arg(CScpiCommandBuilder::formatValue(in_dLevelVolts));
    return sendCommand(in_u32ScopeNumber, cmd);
}

ScopeError CKeysightDSOX2012APlugin::setTriggerMode(U32BIT in_u32ScopeNumber, Enum_TriggerMode in_enumMode)
{
    // :TRIGger:SWEep AUTO|NORMal (single is NORMal + :SINGle at acquire time)
    const QString cmd = QString(":TRIGger:SWEep %1").arg(CScpiCommandBuilder::triggerModeToken(in_enumMode));
    return sendCommand(in_u32ScopeNumber, cmd);
}

// ============================ Acquisition ============================

ScopeError CKeysightDSOX2012APlugin::setAcquireMode(U32BIT in_u32ScopeNumber, Enum_AcquisitionMode in_enumMode, U32BIT in_u32AverageCount)
{
    ScopeError v = CScpiCommandBuilder::validateAcquireMode(getCapabilities(), in_enumMode);
    if (!v.isSuccess()) return v;
    // :ACQuire:TYPE <mode>
    ScopeError err = sendCommand(in_u32ScopeNumber, QString(":ACQuire:TYPE %1").arg(CScpiCommandBuilder::acquireModeToken(in_enumMode)));
    if (err.isSuccess() && in_enumMode == Enum_AcquisitionMode::Average && in_u32AverageCount > 0)
    {
        err = sendCommand(in_u32ScopeNumber, QString(":ACQuire:COUNt %1").arg(in_u32AverageCount));
    }
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_ACQUIRE_FAILED, err.description());
}

ScopeError CKeysightDSOX2012APlugin::run(U32BIT in_u32ScopeNumber)     { return sendCommand(in_u32ScopeNumber, ":RUN"); }
ScopeError CKeysightDSOX2012APlugin::stop(U32BIT in_u32ScopeNumber)    { return sendCommand(in_u32ScopeNumber, ":STOP"); }
ScopeError CKeysightDSOX2012APlugin::single(U32BIT in_u32ScopeNumber)  { return sendCommand(in_u32ScopeNumber, ":SINGle"); }
ScopeError CKeysightDSOX2012APlugin::forceTrigger(U32BIT in_u32ScopeNumber) { return sendCommand(in_u32ScopeNumber, ":TRIGger:FORCe"); }

// ============================ Measurements ============================

ScopeError CKeysightDSOX2012APlugin::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                             Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;

    // :MEASure:<type>? <source>
    const QString query = QString(":MEASure:%1? %2")
                              .arg(CScpiCommandBuilder::measurementToken(in_enumType))
                              .arg(channelSource(in_u32Channel));
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, query, qstrResp);
    if (!err.isSuccess())
    {
        return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());
    }
    bool ok = false;
    const double val = qstrResp.toDouble(&ok);
    if (!ok)
    {
        return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Non-numeric measurement: " + qstrResp);
    }
    // Keysight returns +9.9E+37 for an unmeasurable result.
    if (val > 9.9e37)
    {
        return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Measurement unavailable (no signal)");
    }
    out_dValue = val;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ============================ Waveform ============================

ScopeError CKeysightDSOX2012APlugin::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(getCapabilities(), in_u32Channel);
    if (!v.isSuccess()) return v;

    // Configure the waveform subsystem (Programmer's Guide: :WAVeform).
    ScopeError err = sendCommand(in_u32ScopeNumber, QString(":WAVeform:SOURce %1").arg(channelSource(in_u32Channel)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:FORMat WORD");
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:BYTeorder LSBFirst");
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:UNSigned ON");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    // Fetch and parse the preamble (10 comma-separated fields).
    QString qstrPre;
    err = sendQuery(in_u32ScopeNumber, ":WAVeform:PREamble?", qstrPre);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    const QStringList fields = qstrPre.split(',');
    if (fields.size() < 10)
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Malformed preamble: " + qstrPre);
    }
    S_WaveformPreamble& pre = out_sWaveform.m_sPreamble;
    pre.m_enumFormat  = Enum_WaveformFormat::Word;
    pre.m_u32Points   = fields.at(2).toUInt();
    pre.m_dXIncrement = fields.at(4).toDouble();
    pre.m_dXOrigin    = fields.at(5).toDouble();
    pre.m_dXReference = fields.at(6).toDouble();
    pre.m_dYIncrement = fields.at(7).toDouble();
    pre.m_dYOrigin    = fields.at(8).toDouble();
    pre.m_dYReference = fields.at(9).toDouble();

    // Fetch the raw sample block.
    err = sendCommand(in_u32ScopeNumber, ":WAVeform:DATA?");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    QByteArray baData;
    err = readBinaryBlock(in_u32ScopeNumber, baData);
    if (!err.isSuccess()) return err;

    // Decode unsigned 16-bit LSB-first codes into (time, volts).
    const int iSamples = baData.size() / 2;
    out_sWaveform.m_u32Source = in_u32Channel;
    out_sWaveform.m_dTime.reserve(iSamples);
    out_sWaveform.m_dVolts.reserve(iSamples);

    const unsigned char* p = reinterpret_cast<const unsigned char*>(baData.constData());
    for (int i = 0; i < iSamples; ++i)
    {
        const quint16 raw = static_cast<quint16>(p[2 * i] | (p[2 * i + 1] << 8));
        const double volts = (static_cast<double>(raw) - pre.m_dYReference) * pre.m_dYIncrement + pre.m_dYOrigin;
        const double time  = (static_cast<double>(i)   - pre.m_dXReference) * pre.m_dXIncrement + pre.m_dXOrigin;
        out_sWaveform.m_dVolts.append(volts);
        out_sWaveform.m_dTime.append(time);
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ============================ Status / error ============================

ScopeError CKeysightDSOX2012APlugin::drainSystemError(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus)
{
    // Pull the SCPI error queue dry (:SYSTem:ERRor?), accumulating messages.
    for (int guard = 0; guard < 64; ++guard)
    {
        QString qstrResp;
        ScopeError err = sendQuery(in_u32ScopeNumber, ":SYSTem:ERRor?", qstrResp);
        if (!err.isSuccess()) return err;

        // Response form: <code>,"<message>"
        const int comma = qstrResp.indexOf(',');
        const int code = (comma > 0) ? qstrResp.left(comma).toInt() : qstrResp.toInt();
        if (code == 0) break; // 0,"No error" -> queue empty

        if (!out_sStatus.m_StrErrorMessage.isEmpty()) out_sStatus.m_StrErrorMessage += "; ";
        out_sStatus.m_StrErrorMessage += qstrResp;

        // Negative codes are IEEE 488.2/SCPI command/execution errors.
        if (code <= -100 && code > -200)      out_sStatus.m_statusFlags |= DeviceStatusFlag::CommandError;
        else if (code <= -200 && code > -300) out_sStatus.m_statusFlags |= DeviceStatusFlag::ExecutionError;
        else if (code <= -400 && code > -500) out_sStatus.m_statusFlags |= DeviceStatusFlag::QueryError;
        else                                  out_sStatus.m_statusFlags |= DeviceStatusFlag::HardwareError;
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CKeysightDSOX2012APlugin::readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus)
{
    out_sStatus = S_DeviceErrorStatus();

    U32BIT esr = 0;
    ScopeError err = readStandardEventStatus(in_u32ScopeNumber, esr);
    if (err.isSuccess()) out_sStatus.m_iStandardEventStatus = static_cast<int>(esr);

    U32BIT ques = 0;
    if (readQuestionableStatus(in_u32ScopeNumber, ques).isSuccess())
        out_sStatus.m_iQuestionableStatus = static_cast<int>(ques);

    U32BIT oper = 0;
    if (readOperationStatus(in_u32ScopeNumber, oper).isSuccess())
        out_sStatus.m_iOperationStatus = static_cast<int>(oper);

    return drainSystemError(in_u32ScopeNumber, out_sStatus);
}

ScopeError CKeysightDSOX2012APlugin::clearErrorStatus(U32BIT in_u32ScopeNumber)
{
    return sendCommand(in_u32ScopeNumber, "*CLS");
}

ScopeError CKeysightDSOX2012APlugin::queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage)
{
    return sendQuery(in_u32ScopeNumber, ":SYSTem:ERRor?", out_qstrErrorMessage);
}

ScopeError CKeysightDSOX2012APlugin::readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, "*ESR?", qstrResp);
    if (err.isSuccess()) out_u32Status = qstrResp.toUInt();
    return err;
}

ScopeError CKeysightDSOX2012APlugin::readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, "*STB?", qstrResp);
    if (err.isSuccess()) out_u32Status = qstrResp.toUInt();
    return err;
}

ScopeError CKeysightDSOX2012APlugin::readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, ":STATus:QUEStionable:EVENt?", qstrResp);
    if (err.isSuccess()) out_u32Status = qstrResp.toUInt();
    return err;
}

ScopeError CKeysightDSOX2012APlugin::readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString qstrResp;
    ScopeError err = sendQuery(in_u32ScopeNumber, ":STATus:OPERation:EVENt?", qstrResp);
    if (err.isSuccess()) out_u32Status = qstrResp.toUInt();
    return err;
}
