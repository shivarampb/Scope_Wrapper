// ScopeFamilies.cpp
//
// Dialect presets + family-base overrides. SCPI per vendor programming guides:
//   Keysight InfiniiVision 2000/6000/7000 X-Series Programmer's Guide
//   Keysight Infiniium Programmer's Guide
//   Tektronix TDS1000B/2000C & 3/4/5 Series MDO Programmer Manuals
//   Rohde & Schwarz RTM3000 / RTO Remote Control Manuals
//   Teledyne LeCroy MAUI Oscilloscopes Remote Control Manual
//
// Waveform/measurement paths for Tek/R&S/LeCroy are coded to each manual and
// are the reviewed-map targets; they require hardware/simulator validation
// (Phase 4 of the development plan).

#include "ScopeFamilies.h"
#include "CScpiCommandBuilder.h"
#include <QStringList>
#include <cstring>

// ================================================================
// Construction helpers
// ================================================================

S_PluginInfo makeScopePluginInfo(const char* in_szName, const char* in_szVersion,
                                 const char* in_szManufacturer, const char* in_szModel,
                                 const char* in_szDescription, const QStringList& in_protocols)
{
    S_PluginInfo info;
    strncpy(info.m_szName,           in_szName,         PLUGIN_INFO_NAME_SIZE - 1);
    strncpy(info.m_szVersion,        in_szVersion,      PLUGIN_INFO_VERSION_SIZE - 1);
    strncpy(info.m_szManufacturer,   in_szManufacturer, PLUGIN_INFO_MANUFACTURER_SIZE - 1);
    strncpy(info.m_szModelName,      in_szModel,        PLUGIN_INFO_MODEL_NAME_SIZE - 1);
    strncpy(info.m_szDescription,    in_szDescription,  PLUGIN_INFO_DESCRIPTION_SIZE - 1);
    strncpy(info.m_szMinCoreVersion, "1.0.0",           PLUGIN_INFO_MIN_CORE_VERSION - 1);
    strncpy(info.m_szMaxCoreVersion, "2.0.0",           PLUGIN_INFO_MAX_CORE_VERSION - 1);
    info.m_StrlstSupportedProtocols = in_protocols;
    return info;
}

S_ScopeCapabilities makeScopeCaps(U32BIT in_u32Channels, FDOUBLE in_dBandwidthHz,
                                  FDOUBLE in_dMaxSampleRate, U64BIT in_u64MemoryDepth,
                                  FDOUBLE in_dMinSecPerDiv, FDOUBLE in_dMaxSecPerDiv,
                                  FDOUBLE in_dMinVoltsPerDiv, FDOUBLE in_dMaxVoltsPerDiv,
                                  bool in_bHasGndCoupling, bool in_bIsMSO,
                                  U32BIT in_u32DigitalChannels)
{
    S_ScopeCapabilities caps;
    caps.m_u32NumberOfChannels = in_u32Channels;
    caps.m_u32DigitalChannels  = in_u32DigitalChannels;
    caps.m_dBandwidthHz        = in_dBandwidthHz;
    caps.m_dMaxSampleRate      = in_dMaxSampleRate;
    caps.m_u64MaxMemoryDepth   = in_u64MemoryDepth;
    caps.m_dMinSecondsPerDiv   = in_dMinSecPerDiv;
    caps.m_dMaxSecondsPerDiv   = in_dMaxSecPerDiv;
    caps.m_bHasFFT             = true;
    caps.m_bIsMSO              = in_bIsMSO;

    caps.QlistTriggerTypes << Enum_TriggerType::Edge << Enum_TriggerType::Pulse;
    caps.QlistAcquisitionModes << Enum_AcquisitionMode::Sample
                               << Enum_AcquisitionMode::PeakDetect
                               << Enum_AcquisitionMode::Average
                               << Enum_AcquisitionMode::HighResolution;

    for (U32BIT ch = 1; ch <= in_u32Channels; ++ch)
    {
        S_ChannelCapabilities c;
        c.m_u32ChannelNumber     = ch;
        c.m_dMinVoltsPerDiv      = in_dMinVoltsPerDiv;
        c.m_dMaxVoltsPerDiv      = in_dMaxVoltsPerDiv;
        c.m_dMaxOffset           = 20.0;
        c.m_dMaxProbeAttenuation = 1000.0;
        c.m_bHasBandwidthLimit   = true;
        c.m_bHasACCoupling       = true;
        c.m_bHasGNDCoupling      = in_bHasGndCoupling;
        c.m_bIsDigital           = false;
        caps.QlistChannels.append(c);
    }
    return caps;
}

// ================================================================
// Dialect factories
// ================================================================

static void addStdAcq(S_ScpiDialect& d, const QString& s, const QString& pk, const QString& av, const QString& hr)
{
    d.m_mapAcqTokens[Enum_AcquisitionMode::Sample]         = s;
    d.m_mapAcqTokens[Enum_AcquisitionMode::PeakDetect]     = pk;
    d.m_mapAcqTokens[Enum_AcquisitionMode::Average]        = av;
    d.m_mapAcqTokens[Enum_AcquisitionMode::HighResolution] = hr;
}

S_ScpiDialect keysightInfiniiVisionDialect()
{
    S_ScpiDialect d;
    d.m_strChannelSourceFmt = "CHANnel%1";
    d.m_strIdn = "*IDN?"; d.m_strReset = "*RST"; d.m_strClear = "*CLS"; d.m_strOpcQuery = "*OPC?";
    d.m_strAutoscale = ":AUToscale"; d.m_strSysError = ":SYSTem:ERRor?";
    d.m_strEsrQuery = "*ESR?"; d.m_strStbQuery = "*STB?";
    d.m_strQuesEvent = ":STATus:QUEStionable:EVENt?"; d.m_strOperEvent = ":STATus:OPERation:EVENt?";
    d.m_strChEnable = ":%1:DISPlay %2";
    d.m_strVScale   = ":%1:SCALe %2";
    d.m_strVOffset  = ":%1:OFFSet %2";
    d.m_strCoupling = ":%1:COUPling %2";
    d.m_strProbe    = ":%1:PROBe %2";
    d.m_strBwLimit  = ":%1:BWLimit %2";
    d.m_strTimebaseScale = ":TIMebase:SCALe %1";
    d.m_strTimebasePos   = ":TIMebase:POSition %1";
    d.m_strTrigSource = ":TRIGger:EDGE:SOURce %1";
    d.m_strTrigSlope  = ":TRIGger:EDGE:SLOPe %1";
    d.m_strTrigLevel  = ":TRIGger:EDGE:LEVel %1";
    d.m_strTrigSweep  = ":TRIGger:SWEep %1";
    d.m_strAcqType  = ":ACQuire:TYPE %1";
    d.m_strAcqCount = ":ACQuire:COUNt %1";
    d.m_strRun = ":RUN"; d.m_strStop = ":STOP"; d.m_strSingle = ":SINGle"; d.m_strForce = ":TRIGger:FORCe";
    d.m_strMeasure = ":MEASure:%1? %2";
    d.m_strOn = "ON"; d.m_strOff = "OFF"; d.m_strBwOn = "ON"; d.m_strBwOff = "OFF";
    d.m_strSlopeRising = "POSitive"; d.m_strSlopeFalling = "NEGative"; d.m_strSlopeEither = "EITHer";
    d.m_strSweepAuto = "AUTO"; d.m_strSweepNormal = "NORMal";
    d.m_strCouplingDC = "DC"; d.m_strCouplingAC = "AC"; d.m_strCouplingGND = "GND";
    addStdAcq(d, "NORMal", "PEAK", "AVERage", "HRESolution");
    d.m_mapMeasTokens[Enum_MeasurementType::Vpp]        = "VPP";
    d.m_mapMeasTokens[Enum_MeasurementType::Vamplitude] = "VAMPlitude";
    d.m_mapMeasTokens[Enum_MeasurementType::Vrms]       = "VRMS";
    d.m_mapMeasTokens[Enum_MeasurementType::Vmax]       = "VMAX";
    d.m_mapMeasTokens[Enum_MeasurementType::Vmin]       = "VMIN";
    d.m_mapMeasTokens[Enum_MeasurementType::Vaverage]   = "VAVerage";
    d.m_mapMeasTokens[Enum_MeasurementType::Frequency]  = "FREQuency";
    d.m_mapMeasTokens[Enum_MeasurementType::Period]     = "PERiod";
    d.m_mapMeasTokens[Enum_MeasurementType::RiseTime]   = "RISetime";
    d.m_mapMeasTokens[Enum_MeasurementType::FallTime]   = "FALLtime";
    d.m_mapMeasTokens[Enum_MeasurementType::DutyCycle]  = "DUTYcycle";
    d.m_mapMeasTokens[Enum_MeasurementType::PulseWidth] = "PWIDth";
    d.m_mapMeasTokens[Enum_MeasurementType::Overshoot]  = "OVERshoot";
    return d;
}

S_ScpiDialect keysightInfiniiumDialect()
{
    // Infiniium shares the InfiniiVision command shape for these core ops.
    S_ScpiDialect d = keysightInfiniiVisionDialect();
    d.m_strAutoscale = ":AUToscale";
    // Infiniium acquisition modes: ETIMe/RTIMe differ, but SAMPle/AVERage/HRES map cleanly.
    addStdAcq(d, "NORMal", "PEAK", "AVERage", "HRESolution");
    return d;
}

S_ScpiDialect tektronixDialect()
{
    S_ScpiDialect d;
    d.m_strChannelSourceFmt = "CH%1";
    d.m_strIdn = "*IDN?"; d.m_strReset = "*RST"; d.m_strClear = "*CLS"; d.m_strOpcQuery = "*OPC?";
    d.m_strAutoscale = "AUTOSet EXECute"; d.m_strSysError = "EVMsg?";
    d.m_strEsrQuery = "*ESR?"; d.m_strStbQuery = "*STB?";
    d.m_strQuesEvent = ""; d.m_strOperEvent = "";  // Tek does not expose SCPI QUES/OPER
    d.m_strChEnable = "SELect:%1 %2";
    d.m_strVScale   = "%1:SCAle %2";
    d.m_strVOffset  = "%1:OFFSet %2";
    d.m_strCoupling = "%1:COUPling %2";
    d.m_strProbe    = "%1:PRObe %2";
    d.m_strBwLimit  = "%1:BANdwidth %2";
    d.m_strTimebaseScale = "HORizontal:SCAle %1";
    d.m_strTimebasePos   = "HORizontal:DELay:TIMe %1";
    d.m_strTrigSource = "TRIGger:A:EDGE:SOURce %1";
    d.m_strTrigSlope  = "TRIGger:A:EDGE:SLOpe %1";
    d.m_strTrigLevel  = "TRIGger:A:LEVel %1";
    d.m_strTrigSweep  = "TRIGger:A:MODe %1";
    d.m_strAcqType  = "ACQuire:MODe %1";
    d.m_strAcqCount = "ACQuire:NUMAVg %1";
    d.m_strRun = "ACQuire:STATE RUN"; d.m_strStop = "ACQuire:STATE STOP";
    d.m_strSingle = "ACQuire:STOPAfter SEQuence;:ACQuire:STATE ON"; d.m_strForce = "TRIGger FORCe";
    d.m_strMeasure = "";  // Tek uses MEASUrement:IMMed (override)
    d.m_strOn = "ON"; d.m_strOff = "OFF"; d.m_strBwOn = "TWEnty"; d.m_strBwOff = "FULl";
    d.m_strSlopeRising = "RISe"; d.m_strSlopeFalling = "FALL"; d.m_strSlopeEither = "EITher";
    d.m_strSweepAuto = "AUTO"; d.m_strSweepNormal = "NORMal";
    d.m_strCouplingDC = "DC"; d.m_strCouplingAC = "AC"; d.m_strCouplingGND = "GND";
    addStdAcq(d, "SAMple", "PEAKdetect", "AVErage", "HIRes");
    return d;
}

S_ScpiDialect rohdeSchwarzDialect()
{
    S_ScpiDialect d;
    d.m_strChannelSourceFmt = "CHANnel%1";
    d.m_strIdn = "*IDN?"; d.m_strReset = "*RST"; d.m_strClear = "*CLS"; d.m_strOpcQuery = "*OPC?";
    d.m_strAutoscale = "AUToscale"; d.m_strSysError = "SYSTem:ERRor:NEXT?";
    d.m_strEsrQuery = "*ESR?"; d.m_strStbQuery = "*STB?";
    d.m_strQuesEvent = "STATus:QUEStionable:EVENt?"; d.m_strOperEvent = "STATus:OPERation:EVENt?";
    d.m_strChEnable = "%1:STATe %2";
    d.m_strVScale   = "%1:SCALe %2";
    d.m_strVOffset  = "%1:OFFSet %2";
    d.m_strCoupling = "%1:COUPling %2";
    d.m_strProbe    = "PROBe%1:SETup:ATTenuation:MANual %2";
    d.m_strBwLimit  = "%1:BANDwidth %2";
    d.m_strTimebaseScale = "TIMebase:SCALe %1";
    d.m_strTimebasePos   = "TIMebase:POSition %1";
    d.m_strTrigSource = "TRIGger:A:SOURce %1";
    d.m_strTrigSlope  = "TRIGger:A:EDGE:SLOPe %1";
    d.m_strTrigLevel  = "TRIGger:A:LEVel1 %1";
    d.m_strTrigSweep  = "TRIGger:A:MODE %1";
    d.m_strAcqType  = "ACQuire:MODe %1";      // R&S uses different arm; ACQuire:TYPE via ARIThmetics
    d.m_strAcqCount = "ACQuire:COUNt %1";
    d.m_strRun = "RUN"; d.m_strStop = "STOP"; d.m_strSingle = "SINGle"; d.m_strForce = "TRIGger:FORCe";
    d.m_strMeasure = "MEASurement%2:%1?";   // NOTE: R&S measurement is slot-based (override for real use)
    d.m_strOn = "ON"; d.m_strOff = "OFF"; d.m_strBwOn = "B20"; d.m_strBwOff = "FULL";
    d.m_strSlopeRising = "POSitive"; d.m_strSlopeFalling = "NEGative"; d.m_strSlopeEither = "EITHer";
    d.m_strSweepAuto = "AUTO"; d.m_strSweepNormal = "NORMal";
    d.m_strCouplingDC = "DCLimit"; d.m_strCouplingAC = "ACLimit"; d.m_strCouplingGND = "GND";
    addStdAcq(d, "SAMPle", "PDETect", "AVERage", "HRESolution");
    d.m_mapMeasTokens[Enum_MeasurementType::Vpp]       = "PEAK";
    d.m_mapMeasTokens[Enum_MeasurementType::Frequency] = "FREQuency";
    d.m_mapMeasTokens[Enum_MeasurementType::Period]    = "PERiod";
    d.m_mapMeasTokens[Enum_MeasurementType::Vrms]      = "RMS";
    d.m_mapMeasTokens[Enum_MeasurementType::RiseTime]  = "RTIMe";
    d.m_mapMeasTokens[Enum_MeasurementType::FallTime]  = "FTIMe";
    return d;
}

S_ScpiDialect leCroyDialect()
{
    S_ScpiDialect d;
    d.m_strChannelSourceFmt = "C%1";
    d.m_strIdn = "*IDN?"; d.m_strReset = "*RST"; d.m_strClear = "*CLS"; d.m_strOpcQuery = "*OPC?";
    d.m_strAutoscale = "ASET"; d.m_strSysError = "CMR?";  // LeCroy command-error status
    d.m_strEsrQuery = "*ESR?"; d.m_strStbQuery = "*STB?";
    d.m_strQuesEvent = ""; d.m_strOperEvent = "";
    d.m_strChEnable = "%1:TRAce %2";
    d.m_strVScale   = "%1:VDIV %2";
    d.m_strVOffset  = "%1:OFST %2";
    d.m_strCoupling = "%1:CPL %2";
    d.m_strProbe    = "%1:ATTN %2";
    d.m_strBwLimit  = "BWL %1,%2";
    d.m_strTimebaseScale = "TDIV %1";
    d.m_strTimebasePos   = "TRDL %1";
    d.m_strTrigSource = "TRSE EDGE,SR,%1";  // trigger source (override sets slope/level per source)
    d.m_strTrigSlope  = "%1";               // handled in override
    d.m_strTrigLevel  = "%1";               // handled in override
    d.m_strTrigSweep  = "TRMD %1";
    d.m_strAcqType  = "";                    // LeCroy averaging is a math trace (override/limited)
    d.m_strAcqCount = "";
    d.m_strRun = "TRMD AUTO"; d.m_strStop = "STOP"; d.m_strSingle = "TRMD SINGLE"; d.m_strForce = "FRTR";
    d.m_strMeasure = "";                     // PAVA (override)
    d.m_strOn = "ON"; d.m_strOff = "OFF"; d.m_strBwOn = "ON"; d.m_strBwOff = "OFF";
    d.m_strSlopeRising = "POS"; d.m_strSlopeFalling = "NEG"; d.m_strSlopeEither = "POS";
    d.m_strSweepAuto = "AUTO"; d.m_strSweepNormal = "NORM";
    d.m_strCouplingDC = "D1M"; d.m_strCouplingAC = "A1M"; d.m_strCouplingGND = "GND";
    d.m_mapMeasTokens[Enum_MeasurementType::Vpp]       = "PKPK";
    d.m_mapMeasTokens[Enum_MeasurementType::Vamplitude]= "AMPL";
    d.m_mapMeasTokens[Enum_MeasurementType::Vrms]      = "RMS";
    d.m_mapMeasTokens[Enum_MeasurementType::Vmax]      = "MAX";
    d.m_mapMeasTokens[Enum_MeasurementType::Vmin]      = "MIN";
    d.m_mapMeasTokens[Enum_MeasurementType::Frequency] = "FREQ";
    d.m_mapMeasTokens[Enum_MeasurementType::Period]    = "PER";
    d.m_mapMeasTokens[Enum_MeasurementType::RiseTime]  = "RISE";
    d.m_mapMeasTokens[Enum_MeasurementType::FallTime]  = "FALL";
    d.m_mapMeasTokens[Enum_MeasurementType::DutyCycle] = "DUTY";
    return d;
}

// ================================================================
// Tektronix family overrides
// ================================================================

ScopeError CTektronixScopeBase::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                        Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;

    // Map to Tek MEASUrement:IMMed type tokens.
    QString type;
    switch (in_enumType)
    {
    case Enum_MeasurementType::Vpp:        type = "PK2Pk";     break;
    case Enum_MeasurementType::Vamplitude: type = "AMPlitude"; break;
    case Enum_MeasurementType::Vrms:       type = "RMS";       break;
    case Enum_MeasurementType::Vmax:       type = "MAXimum";   break;
    case Enum_MeasurementType::Vmin:       type = "MINImum";   break;
    case Enum_MeasurementType::Vaverage:   type = "MEAN";      break;
    case Enum_MeasurementType::Frequency:  type = "FREQuency"; break;
    case Enum_MeasurementType::Period:     type = "PERIod";    break;
    case Enum_MeasurementType::RiseTime:   type = "RISe";      break;
    case Enum_MeasurementType::FallTime:   type = "FALL";      break;
    case Enum_MeasurementType::DutyCycle:  type = "PDUty";     break;
    case Enum_MeasurementType::PulseWidth: type = "PWIdth";    break;
    default: return ScopeError(ScopeErrorCode::INVALID_MEASUREMENT);
    }

    ScopeError err = sendCommand(in_u32ScopeNumber, QString("MEASUrement:IMMed:SOUrce %1").arg(m_dialect.source(in_u32Channel)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, QString("MEASUrement:IMMed:TYPe %1").arg(type));
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());

    QString resp;
    err = sendQuery(in_u32ScopeNumber, "MEASUrement:IMMed:VALue?", resp);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());
    bool ok = false;
    const double val = resp.toDouble(&ok);
    if (!ok) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Non-numeric: " + resp);
    if (val > 9.9e37) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Measurement unavailable");
    out_dValue = val;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CTektronixScopeBase::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;

    // Configure transfer: signed 16-bit big-endian binary (Tek RIBinary).
    ScopeError err = sendCommand(in_u32ScopeNumber, QString("DATa:SOUrce %1").arg(m_dialect.source(in_u32Channel)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, "DATa:ENCdg RIBinary");
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, "DATa:WIDth 2");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    // Read scaling factors from the waveform preamble (individual queries).
    QString sXincr, sXzero, sYmult, sYoff, sYzero, sPoints;
    if (sendQuery(in_u32ScopeNumber, "WFMOutpre:XINcr?",  sXincr ).isSuccess() &&
        sendQuery(in_u32ScopeNumber, "WFMOutpre:XZEro?",  sXzero ).isSuccess() &&
        sendQuery(in_u32ScopeNumber, "WFMOutpre:YMUlt?",  sYmult ).isSuccess() &&
        sendQuery(in_u32ScopeNumber, "WFMOutpre:YOFf?",   sYoff  ).isSuccess() &&
        sendQuery(in_u32ScopeNumber, "WFMOutpre:YZEro?",  sYzero ).isSuccess() &&
        sendQuery(in_u32ScopeNumber, "WFMOutpre:NR_Pt?",  sPoints).isSuccess())
    {
        // ok
    }
    else
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Failed to read WFMOutpre");
    }

    S_WaveformPreamble& p = out_sWaveform.m_sPreamble;
    p.m_enumFormat  = Enum_WaveformFormat::Word;
    p.m_dXIncrement = sXincr.toDouble();
    p.m_dXOrigin    = sXzero.toDouble();
    p.m_dXReference = 0.0;
    p.m_dYIncrement = sYmult.toDouble();
    p.m_dYReference = sYoff.toDouble();
    p.m_dYOrigin    = sYzero.toDouble();
    p.m_u32Points   = sPoints.toUInt();

    err = sendCommand(in_u32ScopeNumber, "CURVe?");
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
        // Big-endian signed 16-bit (RIBinary).
        const qint16 raw = static_cast<qint16>((pd[2*i] << 8) | pd[2*i+1]);
        // Tek scaling: volts = (raw - YOFf) * YMUlt + YZEro
        out_sWaveform.m_dVolts.append((static_cast<double>(raw) - p.m_dYReference) * p.m_dYIncrement + p.m_dYOrigin);
        out_sWaveform.m_dTime.append(p.m_dXOrigin + static_cast<double>(i) * p.m_dXIncrement);
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ================================================================
// Rohde & Schwarz family overrides
// ================================================================

ScopeError CRohdeSchwarzScopeBase::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                           Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    if (!m_dialect.m_mapMeasTokens.contains(in_enumType))
        return ScopeError(ScopeErrorCode::INVALID_MEASUREMENT, "Measurement not supported");

    // R&S measurements are slot-based: configure slot 1 then read its result.
    const QString src = m_dialect.source(in_u32Channel);
    ScopeError err = sendCommand(in_u32ScopeNumber, QString("MEASurement1:SOURce %1").arg(src));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, QString("MEASurement1:MAIN %1").arg(m_dialect.m_mapMeasTokens.value(in_enumType)));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, "MEASurement1:ENABle ON");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());

    QString resp;
    err = sendQuery(in_u32ScopeNumber, "MEASurement1:RESult:ACTual?", resp);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());
    bool ok = false;
    const double val = resp.toDouble(&ok);
    if (!ok) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Non-numeric: " + resp);
    out_dValue = val;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CRohdeSchwarzScopeBase::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    const QString src = m_dialect.source(in_u32Channel);

    // 32-bit float samples (volts) via a definite-length binary block.
    ScopeError err = sendCommand(in_u32ScopeNumber, "FORMat:DATA REAL,32");
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    // Header: "XStart,XStop,RecordLength,ValuesPerSample"
    QString header;
    err = sendQuery(in_u32ScopeNumber, QString("%1:DATA:HEADer?").arg(src), header);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());
    const QStringList h = header.split(',');
    if (h.size() < 3) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "Malformed header: " + header);
    const double xStart = h.at(0).toDouble();
    const double xStop  = h.at(1).toDouble();
    const int    recLen = h.at(2).toInt();

    err = sendCommand(in_u32ScopeNumber, QString("%1:DATA?").arg(src));
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, err.description());

    QByteArray data;
    err = readBinaryBlock(in_u32ScopeNumber, data);
    if (!err.isSuccess()) return err;

    const int n = data.size() / 4;
    const double dx = (recLen > 1) ? (xStop - xStart) / static_cast<double>(recLen - 1) : 0.0;
    out_sWaveform.m_u32Source = in_u32Channel;
    out_sWaveform.m_sPreamble.m_enumFormat  = Enum_WaveformFormat::Word;
    out_sWaveform.m_sPreamble.m_u32Points   = static_cast<U32BIT>(n);
    out_sWaveform.m_sPreamble.m_dXOrigin    = xStart;
    out_sWaveform.m_sPreamble.m_dXIncrement = dx;
    out_sWaveform.m_dTime.reserve(n);
    out_sWaveform.m_dVolts.reserve(n);
    const char* pd = data.constData();
    for (int i = 0; i < n; ++i)
    {
        float fv = 0.0f;
        std::memcpy(&fv, pd + 4 * i, 4);  // R&S REAL,32 little-endian by default
        out_sWaveform.m_dVolts.append(static_cast<double>(fv));
        out_sWaveform.m_dTime.append(xStart + static_cast<double>(i) * dx);
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

// ================================================================
// LeCroy family overrides
// ================================================================

ScopeError CLeCroyScopeBase::setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_sTrigger.m_u32Source);
    if (!v.isSuccess()) return ScopeError(ScopeErrorCode::INVALID_SOURCE, v.description());
    const QString src = m_dialect.source(in_sTrigger.m_u32Source);

    QString slope = (in_sTrigger.m_enumSlope == Enum_TriggerSlope::Falling)
                        ? m_dialect.m_strSlopeFalling : m_dialect.m_strSlopeRising;

    // TRSE EDGE,SR,C1  /  C1:TRSL POS  /  C1:TRLV <v>  /  TRMD AUTO|NORM
    ScopeError err = sendCommand(in_u32ScopeNumber, QString("TRSE EDGE,SR,%1").arg(src));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, QString("%1:TRSL %2").arg(src).arg(slope));
    if (err.isSuccess()) err = sendCommand(in_u32ScopeNumber, QString("%1:TRLV %2").arg(src).arg(CScpiCommandBuilder::formatValue(in_sTrigger.m_dLevel)));
    if (err.isSuccess()) err = setTriggerMode(in_u32ScopeNumber, in_sTrigger.m_enumMode);
    return err.isSuccess() ? err : ScopeError(ScopeErrorCode::SET_TRIGGER_FAILED, err.description());
}

ScopeError CLeCroyScopeBase::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                     Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    if (!m_dialect.m_mapMeasTokens.contains(in_enumType))
        return ScopeError(ScopeErrorCode::INVALID_MEASUREMENT, "Measurement not supported");

    // C<n>:PAVA? <PARAM>  ->  "<PARAM>,<value> <unit>"
    const QString query = QString("%1:PAVA? %2").arg(m_dialect.source(in_u32Channel)).arg(m_dialect.m_mapMeasTokens.value(in_enumType));
    QString resp;
    ScopeError err = sendQuery(in_u32ScopeNumber, query, resp);
    if (!err.isSuccess()) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, err.description());

    // Parse the numeric field out of "PKPK,1.234E-01 V".
    const QStringList parts = resp.split(',');
    const QString valField = (parts.size() > 1) ? parts.at(1) : resp;
    bool ok = false;
    const double val = valField.section(' ', 0, 0).toDouble(&ok);
    if (!ok) return ScopeError(ScopeErrorCode::READ_MEASUREMENT_FAILED, "Non-numeric: " + resp);
    out_dValue = val;
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CLeCroyScopeBase::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    ScopeError v = CScpiCommandBuilder::validateChannel(m_caps, in_u32Channel);
    if (!v.isSuccess()) return v;
    const QString src = m_dialect.source(in_u32Channel);

    // ASCII path via INSPECT (portable across MAUI models; slower than binary).
    QString sInterval, sOffset, sData;
    if (!sendQuery(in_u32ScopeNumber, QString("%1:INSPECT? 'HORIZ_INTERVAL'").arg(src), sInterval).isSuccess() ||
        !sendQuery(in_u32ScopeNumber, QString("%1:INSPECT? 'HORIZ_OFFSET'").arg(src),   sOffset  ).isSuccess() ||
        !sendQuery(in_u32ScopeNumber, QString("%1:INSPECT? 'SIMPLE'").arg(src),         sData    ).isSuccess())
    {
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "INSPECT query failed");
    }

    // INSPECT returns: C1:INSP "HORIZ_INTERVAL : 2e-09". Extract the number after ':'.
    auto numAfterColon = [](const QString& in) -> double {
        const int c = in.lastIndexOf(':');
        const QString tail = (c >= 0) ? in.mid(c + 1) : in;
        return tail.remove('"').trimmed().toDouble();
    };
    const double dx = numAfterColon(sInterval);
    const double x0 = numAfterColon(sOffset);

    // 'SIMPLE' returns whitespace-separated voltage values (wrapped in quotes).
    QString body = sData;
    const int c = body.indexOf(':');
    if (c >= 0) body = body.mid(c + 1);
    body.remove('"');
    const QStringList tokens = body.simplified().split(' ', Qt::SkipEmptyParts);

    out_sWaveform.m_u32Source = in_u32Channel;
    out_sWaveform.m_sPreamble.m_enumFormat  = Enum_WaveformFormat::Ascii;
    out_sWaveform.m_sPreamble.m_dXIncrement = dx;
    out_sWaveform.m_sPreamble.m_dXOrigin    = x0;
    out_sWaveform.m_sPreamble.m_u32Points   = static_cast<U32BIT>(tokens.size());
    out_sWaveform.m_dTime.reserve(tokens.size());
    out_sWaveform.m_dVolts.reserve(tokens.size());
    for (int i = 0; i < tokens.size(); ++i)
    {
        bool ok = false;
        const double val = tokens.at(i).toDouble(&ok);
        if (!ok) continue;
        out_sWaveform.m_dVolts.append(val);
        out_sWaveform.m_dTime.append(x0 + static_cast<double>(i) * dx);
    }
    if (out_sWaveform.m_dVolts.isEmpty())
        return ScopeError(ScopeErrorCode::WAVEFORM_TRANSFER_FAILED, "No samples parsed");
    return ScopeError(ScopeErrorCode::SUCCESS);
}
