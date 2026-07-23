// CVisaScopePlugin.cpp

#include "CVisaScopePlugin.h"
#include "VisaHelper.h"
#include "CScpiCommandBuilder.h"
#include <QStringList>
#include <QDebug>

CVisaScopePlugin::CVisaScopePlugin() {}

CVisaScopePlugin::~CVisaScopePlugin()
{
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it)
    {
        if (it.value().m_vi != VI_NULL)        viClose(it.value().m_vi);
        if (it.value().m_defaultRM != VI_NULL) viClose(it.value().m_defaultRM);
    }
    m_devices.clear();
}

// ============================ Transport ============================

ScopeError CVisaScopePlugin::visaErrorToScope(ViStatus in_ViStatus, const QString& in_kstrContext) const
{
    if (in_ViStatus == VI_SUCCESS) return ScopeError(ScopeErrorCode::SUCCESS);
    const QString kPrefix = in_kstrContext.isEmpty() ? "" : in_kstrContext + ": ";
    switch (in_ViStatus)
    {
    case VI_ERROR_RSRC_NFOUND:   return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Resource not found");
    case VI_ERROR_TMO:           return ScopeError(ScopeErrorCode::COMMUNICATION_TIMEOUT, kPrefix + "Timeout");
    case VI_ERROR_CONN_LOST:     return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Connection lost");
    case VI_ERROR_INV_RSRC_NAME: return ScopeError(ScopeErrorCode::CONNECTION_FAILED, kPrefix + "Invalid resource name");
    default:
        return ScopeError(ScopeErrorCode::COMMUNICATION_ERROR,
                          kPrefix + QString("VISA error: 0x%1").arg(in_ViStatus, 0, 16));
    }
}

ScopeError CVisaScopePlugin::sendCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd)
{
    if (!isConnected(in_u32ScopeNumber)) return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    S_DeviceInstance& dev = m_devices[in_u32ScopeNumber - 1];

    QString cmd = in_kqstrCmd;
    if (!m_dialect.m_strTerminator.isEmpty() && !cmd.endsWith(m_dialect.m_strTerminator))
        cmd += m_dialect.m_strTerminator;

    QByteArray ba = cmd.toLatin1();
    ViUInt32 ret = 0;
    ViStatus st = viWrite(dev.m_vi, (ViBuf)ba.data(), ba.length(), &ret);
    if (st < VI_SUCCESS) return visaErrorToScope(st, "Failed to write command");
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CVisaScopePlugin::sendQuery(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd, QString& out_qstrResponse)
{
    if (!isConnected(in_u32ScopeNumber)) return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    S_DeviceInstance& dev = m_devices[in_u32ScopeNumber - 1];

    QString cmd = in_kqstrCmd;
    if (!m_dialect.m_strTerminator.isEmpty() && !cmd.endsWith(m_dialect.m_strTerminator))
        cmd += m_dialect.m_strTerminator;

    QByteArray ba = cmd.toLatin1();
    ViUInt32 ret = 0;
    ViStatus st = viWrite(dev.m_vi, (ViBuf)ba.data(), ba.length(), &ret);
    if (st < VI_SUCCESS) return visaErrorToScope(st, "Failed to write query");

    // Loop-read until end-of-message so long ASCII responses are not truncated.
    QByteArray resp;
    char buf[8192];
    do {
        st = viRead(dev.m_vi, (ViBuf)buf, sizeof(buf), &ret);
        if (st < VI_SUCCESS && st != VI_SUCCESS_MAX_CNT && st != VI_ERROR_TMO && ret == 0)
            return visaErrorToScope(st, "Failed to read response");
        resp.append(buf, static_cast<int>(ret));
    } while (st == VI_SUCCESS_MAX_CNT);

    out_qstrResponse = QString::fromLatin1(resp).trimmed();
    if (out_qstrResponse.isEmpty())
        return ScopeError(ScopeErrorCode::INVALID_RESPONSE, "Empty response");
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CVisaScopePlugin::readBinaryBlock(U32BIT in_u32ScopeNumber, QByteArray& out_baData)
{
    out_baData.clear();
    S_DeviceInstance& dev = m_devices[in_u32ScopeNumber - 1];

    QByteArray raw;
    char chunk[8192];
    ViUInt32 ret = 0;
    ViStatus st = VI_SUCCESS;
    do {
        st = viRead(dev.m_vi, (ViBuf)chunk, sizeof(chunk), &ret);
        if (st < VI_SUCCESS && st != VI_SUCCESS_MAX_CNT && ret == 0)
            return visaErrorToScope(st, "Failed to read binary block");
        raw.append(chunk, static_cast<int>(ret));
    } while (st == VI_SUCCESS_MAX_CNT);

    if (raw.isEmpty() || raw.at(0) != '#')
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Malformed block header");
    const int nDigits = QString(QChar(raw.at(1))).toInt();
    if (nDigits <= 0)
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Bad block length prefix");
    const int len = raw.mid(2, nDigits).toInt();
    out_baData = raw.mid(2 + nDigits, len);
    if (out_baData.size() < len)
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Short binary block");
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CVisaScopePlugin::applyTemplate(U32BIT in_u32ScopeNumber, const QString& in_kTemplateSourceValue,
                                           U32BIT in_u32Channel, const QString& in_kValue)
{
    const QString cmd = in_kTemplateSourceValue.arg(m_dialect.source(in_u32Channel)).arg(in_kValue);
    return sendCommand(in_u32ScopeNumber, cmd);
}

// ============================ Connection ============================

ScopeError CVisaScopePlugin::connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig)
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (m_devices.contains(key) && m_devices[key].m_bConnected)
        return ScopeError(ScopeErrorCode::ALREADY_CONNECTED);

    S_DeviceInstance dev;
    dev.m_sConfig = in_sConfig;

    ViStatus st = viOpenDefaultRM(&dev.m_defaultRM);
    if (st < VI_SUCCESS) return visaErrorToScope(st, "Failed to open resource manager");

    const QString resource = in_sConfig.toVisaResourceString();
    qDebug() << "Connecting to:" << resource;

    st = viOpen(dev.m_defaultRM, resource.toLatin1().data(), VI_NULL, VI_NULL, &dev.m_vi);
    if (st < VI_SUCCESS)
    {
        viClose(dev.m_defaultRM);
        return visaErrorToScope(st, "Failed to open instrument");
    }
    viSetAttribute(dev.m_vi, VI_ATTR_TMO_VALUE, in_sConfig.m_u32Timeout);
    if (in_sConfig.m_enumProtocol == Enum_CommunicationProtocol::RS232)
    {
        viSetAttribute(dev.m_vi, VI_ATTR_ASRL_BAUD, in_sConfig.m_u32BaudRate);
        viSetAttribute(dev.m_vi, VI_ATTR_ASRL_DATA_BITS, 8);
        viSetAttribute(dev.m_vi, VI_ATTR_ASRL_PARITY, VI_ASRL_PAR_NONE);
        viSetAttribute(dev.m_vi, VI_ATTR_ASRL_STOP_BITS, VI_ASRL_STOP_ONE);
    }

    dev.m_bConnected = true;
    m_devices[key] = dev;

    QString idn;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strIdn, idn);
    if (!err.isSuccess())
    {
        disconnect(in_u32ScopeNumber);
        return ScopeError(ScopeErrorCode::CONNECTION_FAILED, "Device not responding");
    }
    qDebug() << "Device identified as:" << idn;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CVisaScopePlugin::disconnect(U32BIT in_u32ScopeNumber)
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (!m_devices.contains(key)) return ScopeError(ScopeErrorCode::NOT_CONNECTED);
    S_DeviceInstance& dev = m_devices[key];
    if (dev.m_vi != VI_NULL)        viClose(dev.m_vi);
    if (dev.m_defaultRM != VI_NULL) viClose(dev.m_defaultRM);
    m_devices.remove(key);
    return ScopeError(ScopeErrorCode::SUCCESS);
}

bool CVisaScopePlugin::isConnected(U32BIT in_u32ScopeNumber) const
{
    const U32BIT key = in_u32ScopeNumber - 1;
    if (!m_devices.contains(key)) return false;
    return m_devices[key].m_bConnected && m_devices[key].m_vi != VI_NULL;
}

ScopeError CVisaScopePlugin::reset(U32BIT in_u32ScopeNumber)
{
    ScopeError err = sendCommand(in_u32ScopeNumber, m_dialect.m_strReset);
    if (err.isSuccess() && !m_dialect.m_strClear.isEmpty())
        err = sendCommand(in_u32ScopeNumber, m_dialect.m_strClear);
    return err;
}

ScopeError CVisaScopePlugin::autoSetup(U32BIT in_u32ScopeNumber)
{
    ScopeError err = sendCommand(in_u32ScopeNumber, m_dialect.m_strAutoscale);
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::AUTOSET_FAILED, err.description());
}

ScopeError CVisaScopePlugin::checkOperationComplete(U32BIT in_u32ScopeNumber, bool& out_bComplete)
{
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strOpcQuery, resp);
    out_bComplete = (err.isSuccess() && resp.trimmed().startsWith("1"));
    return err;
}

// ============================ Vertical ============================

ScopeError CVisaScopePlugin::setChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bEnable)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    return applyTemplate(in_u32ScopeNumber, m_dialect.m_strChEnable, in_u32Channel,
                         in_bEnable ? m_dialect.m_strOn : m_dialect.m_strOff);
}

ScopeError CVisaScopePlugin::setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv)
{
    ScopeError v = CScpiCommandBuilder::validateVerticalScale(m_caps, in_u32Channel, in_dVoltsPerDiv);
    if (!v.isSuccess()) return v;
    ScopeError err = applyTemplate(in_u32ScopeNumber, m_dialect.m_strVScale, in_u32Channel,
                                   CScpiCommandBuilder::formatValue(in_dVoltsPerDiv));
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_VERTICAL_FAILED, err.description());
}

ScopeError CVisaScopePlugin::setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dOffsetVolts)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    return applyTemplate(in_u32ScopeNumber, m_dialect.m_strVOffset, in_u32Channel,
                         CScpiCommandBuilder::formatValue(in_dOffsetVolts));
}

ScopeError CVisaScopePlugin::setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Coupling in_enumCoupling)
{
    ScopeError v = CScpiCommandBuilder::validateCoupling(m_caps, in_u32Channel, in_enumCoupling);
    if (!v.isSuccess()) return v;
    QString tok;
    switch (in_enumCoupling)
    {
    case Enum_Coupling::DC:  tok = m_dialect.m_strCouplingDC;  break;
    case Enum_Coupling::AC:  tok = m_dialect.m_strCouplingAC;  break;
    case Enum_Coupling::GND: tok = m_dialect.m_strCouplingGND; break;
    default:                 tok = m_dialect.m_strCouplingDC;  break;
    }
    return applyTemplate(in_u32ScopeNumber, m_dialect.m_strCoupling, in_u32Channel, tok);
}

ScopeError CVisaScopePlugin::setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dAttenuation)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    return applyTemplate(in_u32ScopeNumber, m_dialect.m_strProbe, in_u32Channel,
                         CScpiCommandBuilder::formatValue(in_dAttenuation));
}

ScopeError CVisaScopePlugin::setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bLimit)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    return applyTemplate(in_u32ScopeNumber, m_dialect.m_strBwLimit, in_u32Channel,
                         in_bLimit ? m_dialect.m_strBwOn : m_dialect.m_strBwOff);
}

// ============================ Horizontal ============================

ScopeError CVisaScopePlugin::setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv)
{
    ScopeError v = CScpiCommandBuilder::validateTimebaseScale(m_caps, in_dSecondsPerDiv);
    if (!v.isSuccess()) return v;
    ScopeError err = sendCommand(in_u32ScopeNumber, m_dialect.m_strTimebaseScale.arg(CScpiCommandBuilder::formatValue(in_dSecondsPerDiv)));
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_TIMEBASE_FAILED, err.description());
}

ScopeError CVisaScopePlugin::setHorizontalPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dDelaySeconds)
{
    return sendCommand(in_u32ScopeNumber, m_dialect.m_strTimebasePos.arg(CScpiCommandBuilder::formatValue(in_dDelaySeconds)));
}

// ============================ Trigger ============================

ScopeError CVisaScopePlugin::setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_sTrigger.m_u32Source);
    if (!v.isSuccess()) return ScopeError(ScopeErrorCode::INVALID_SOURCE, v.description());

    QString slope;
    switch (in_sTrigger.m_enumSlope)
    {
    case Enum_TriggerSlope::Rising:  slope = m_dialect.m_strSlopeRising;  break;
    case Enum_TriggerSlope::Falling: slope = m_dialect.m_strSlopeFalling; break;
    case Enum_TriggerSlope::Either:  slope = m_dialect.m_strSlopeEither;  break;
    }

    ScopeError err = sendCommand(in_u32ScopeNumber, m_dialect.m_strTrigSource.arg(m_dialect.source(in_sTrigger.m_u32Source)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, m_dialect.m_strTrigSlope.arg(slope));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, m_dialect.m_strTrigLevel.arg(CScpiCommandBuilder::formatValue(in_sTrigger.m_dLevel)));
    if (err.isSuccess()) err = setTriggerMode(in_u32ScopeNumber, in_sTrigger.m_enumMode);
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_TRIGGER_FAILED, err.description());
}

ScopeError CVisaScopePlugin::setTriggerLevel(U32BIT in_u32ScopeNumber, FDOUBLE in_dLevelVolts)
{
    return sendCommand(in_u32ScopeNumber, m_dialect.m_strTrigLevel.arg(CScpiCommandBuilder::formatValue(in_dLevelVolts)));
}

ScopeError CVisaScopePlugin::setTriggerMode(U32BIT in_u32ScopeNumber, Enum_TriggerMode in_enumMode)
{
    const QString tok = (in_enumMode == Enum_TriggerMode::Auto) ? m_dialect.m_strSweepAuto : m_dialect.m_strSweepNormal;
    return sendCommand(in_u32ScopeNumber, m_dialect.m_strTrigSweep.arg(tok));
}

// ============================ Acquisition ============================

ScopeError CVisaScopePlugin::setAcquireMode(U32BIT in_u32ScopeNumber, Enum_AcquisitionMode in_enumMode, U32BIT in_u32AverageCount)
{
    ScopeError v = CScpiCommandBuilder::validateAcquireMode(m_caps, in_enumMode);
    if (!v.isSuccess()) return v;
    if (!m_dialect.m_mapAcqTokens.contains(in_enumMode))
        return ScopeError(ScopeErrorCode::INVALID_ACQUIRE_MODE, "Mode not in dialect");

    ScopeError err = sendCommand(in_u32ScopeNumber, m_dialect.m_strAcqType.arg(m_dialect.m_mapAcqTokens.value(in_enumMode)));
    if (err.isSuccess() && in_enumMode == Enum_AcquisitionMode::Average && in_u32AverageCount > 0)
        err = sendCommand(in_u32ScopeNumber, m_dialect.m_strAcqCount.arg(in_u32AverageCount));
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_ACQUIRE_FAILED, err.description());
}

ScopeError CVisaScopePlugin::run(U32BIT in_u32ScopeNumber)          { return sendCommand(in_u32ScopeNumber, m_dialect.m_strRun); }
ScopeError CVisaScopePlugin::stop(U32BIT in_u32ScopeNumber)         { return sendCommand(in_u32ScopeNumber, m_dialect.m_strStop); }
ScopeError CVisaScopePlugin::single(U32BIT in_u32ScopeNumber)       { return sendCommand(in_u32ScopeNumber, m_dialect.m_strSingle); }
ScopeError CVisaScopePlugin::forceTrigger(U32BIT in_u32ScopeNumber) { return sendCommand(in_u32ScopeNumber, m_dialect.m_strForce); }

// ============================ Measurement ============================

ScopeError CVisaScopePlugin::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                     Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    if (!m_dialect.m_mapMeasTokens.contains(in_enumType))
        return ScopeError(ScopeErrorCode::INVALID_MEASUREMENT, "Measurement not in dialect");

    const QString query = m_dialect.m_strMeasure
                              .arg(m_dialect.m_mapMeasTokens.value(in_enumType))
                              .arg(m_dialect.source(in_u32Channel));
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, query, resp);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());

    bool ok = false;
    const double val = resp.toDouble(&ok);
    if (!ok) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Non-numeric: " + resp);
    if (val > 9.9e37) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Measurement unavailable");
    out_dValue = val;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ============================ Waveform (Keysight-style default) ============================

ScopeError CVisaScopePlugin::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;

    ScopeError err = sendCommand(in_u32ScopeNumber, QString(":WAVeform:SOURce %1").arg(m_dialect.source(in_u32Channel)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:FORMat WORD");
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:BYTeorder LSBFirst");
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, ":WAVeform:UNSigned ON");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    QString pre;
    err = sendQuery(in_u32ScopeNumber, ":WAVeform:PREamble?", pre);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());
    const QStringList f = pre.split(',');
    if (f.size() < 10) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Malformed preamble: " + pre);

    S_WaveformPreamble& p = out_sWaveform.m_sPreamble;
    p.m_enumFormat  = Enum_WaveformFormat::Word;
    p.m_u32Points   = f.at(2).toUInt();
    p.m_dXIncrement = f.at(4).toDouble();
    p.m_dXOrigin    = f.at(5).toDouble();
    p.m_dXReference = f.at(6).toDouble();
    p.m_dYIncrement = f.at(7).toDouble();
    p.m_dYOrigin    = f.at(8).toDouble();
    p.m_dYReference = f.at(9).toDouble();

    err = sendCommand(in_u32ScopeNumber, ":WAVeform:DATA?");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    QByteArray data;
    err = readBinaryBlock(in_u32ScopeNumber, data);
    if (!err.isSuccess()) return err;

    const int n = data.size() / 2;
    out_sWaveform.m_u32Source = in_u32Channel;
    out_sWaveform.m_dTime.reserve(n);
    out_sWaveform.m_dVolts.reserve(n);
    const unsigned char* pd = reinterpret_cast<const unsigned char*>(data.constData());
    for (int i = 0; i < n; ++i)
    {
        const quint16 raw = static_cast<quint16>(pd[2*i] | (pd[2*i+1] << 8));
        out_sWaveform.m_dVolts.append((static_cast<double>(raw) - p.m_dYReference) * p.m_dYIncrement + p.m_dYOrigin);
        out_sWaveform.m_dTime.append((static_cast<double>(i) - p.m_dXReference) * p.m_dXIncrement + p.m_dXOrigin);
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ============================ Status / error ============================

ScopeError CVisaScopePlugin::readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus)
{
    out_sStatus = S_DeviceErrorStatus();

    U32BIT esr = 0;
    if (readStandardEventStatus(in_u32ScopeNumber, esr).isSuccess())
        out_sStatus.m_iStandardEventStatus = static_cast<int>(esr);
    U32BIT ques = 0;
    if (readQuestionableStatus(in_u32ScopeNumber, ques).isSuccess())
        out_sStatus.m_iQuestionableStatus = static_cast<int>(ques);
    U32BIT oper = 0;
    if (readOperationStatus(in_u32ScopeNumber, oper).isSuccess())
        out_sStatus.m_iOperationStatus = static_cast<int>(oper);

    // Drain the error queue (:SYSTem:ERRor? style: "<code>,\"<msg>\"").
    for (int guard = 0; guard < 64; ++guard)
    {
        QString resp;
        ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strSysError, resp);
        if (!err.isSuccess()) return err;
        const int comma = resp.indexOf(',');
        const int code = (comma > 0) ? resp.left(comma).toInt() : resp.toInt();
        if (code == 0) break;
        if (!out_sStatus.m_StrErrorMessage.isEmpty()) out_sStatus.m_StrErrorMessage += "; ";
        out_sStatus.m_StrErrorMessage += resp;
        if (code <= -100 && code > -200)      out_sStatus.m_statusFlags |= DeviceStatusFlag::CommandError;
        else if (code <= -200 && code > -300) out_sStatus.m_statusFlags |= DeviceStatusFlag::ExecutionError;
        else if (code <= -400 && code > -500) out_sStatus.m_statusFlags |= DeviceStatusFlag::QueryError;
        else                                  out_sStatus.m_statusFlags |= DeviceStatusFlag::HardwareError;
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CVisaScopePlugin::clearErrorStatus(U32BIT in_u32ScopeNumber)
{
    return sendCommand(in_u32ScopeNumber, m_dialect.m_strClear);
}

ScopeError CVisaScopePlugin::queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage)
{
    return sendQuery(in_u32ScopeNumber, m_dialect.m_strSysError, out_qstrErrorMessage);
}

ScopeError CVisaScopePlugin::readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strEsrQuery, resp);
    if (err.isSuccess()) out_u32Status = resp.toUInt();
    return err;
}

ScopeError CVisaScopePlugin::readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strStbQuery, resp);
    if (err.isSuccess()) out_u32Status = resp.toUInt();
    return err;
}

ScopeError CVisaScopePlugin::readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    out_u32Status = 0;
    if (m_dialect.m_strQuesEvent.isEmpty()) return ScopeError(ScopeErrorCode::SUCCESS); // unsupported
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strQuesEvent, resp);
    if (err.isSuccess()) out_u32Status = resp.toUInt();
    return err;
}

ScopeError CVisaScopePlugin::readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status)
{
    out_u32Status = 0;
    if (m_dialect.m_strOperEvent.isEmpty()) return ScopeError(ScopeErrorCode::SUCCESS); // unsupported
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, m_dialect.m_strOperEvent, resp);
    if (err.isSuccess()) out_u32Status = resp.toUInt();
    return err;
}
