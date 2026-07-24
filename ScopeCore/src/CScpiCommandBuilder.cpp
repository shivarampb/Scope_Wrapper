// CScpiCommandBuilder.cpp

#include "CScpiCommandBuilder.h"

QString CScpiCommandBuilder::formatValue(FDOUBLE in_dValue, int in_iPrecision)
{
    // 'g' keeps commands compact and within instrument-parseable notation,
    // e.g. 2e-08, 0.5, 1.5e+03.
    return QString::number(in_dValue, 'g', in_iPrecision);
}

QString CScpiCommandBuilder::fill(const QString& in_kTemplate, U32BIT in_u32Arg1)
{
    return in_kTemplate.arg(in_u32Arg1);
}

QString CScpiCommandBuilder::fill(const QString& in_kTemplate, U32BIT in_u32Arg1, const QString& in_kArg2)
{
    return in_kTemplate.arg(in_u32Arg1).arg(in_kArg2);
}

QString CScpiCommandBuilder::fill(const QString& in_kTemplate, const QString& in_kArg1)
{
    return in_kTemplate.arg(in_kArg1);
}

QString CScpiCommandBuilder::couplingToken(Enum_Coupling in_enumCoupling)
{
    switch (in_enumCoupling)
    {
    case Enum_Coupling::DC:  return "DC";
    case Enum_Coupling::AC:  return "AC";
    case Enum_Coupling::GND: return "GND";
    default:                 return "DC";
    }
}

QString CScpiCommandBuilder::slopeToken(Enum_TriggerSlope in_enumSlope)
{
    switch (in_enumSlope)
    {
    case Enum_TriggerSlope::Rising:  return "POSitive";
    case Enum_TriggerSlope::Falling: return "NEGative";
    case Enum_TriggerSlope::Either:  return "EITHer";
    default:                         return "POSitive";
    }
}

QString CScpiCommandBuilder::triggerModeToken(Enum_TriggerMode in_enumMode)
{
    switch (in_enumMode)
    {
    case Enum_TriggerMode::Auto:   return "AUTO";
    case Enum_TriggerMode::Normal: return "NORMal";
    case Enum_TriggerMode::Single: return "NORMal"; // single = normal + :SINGle
    default:                       return "AUTO";
    }
}

QString CScpiCommandBuilder::acquireModeToken(Enum_AcquisitionMode in_enumMode)
{
    switch (in_enumMode)
    {
    case Enum_AcquisitionMode::Sample:         return "NORMal";
    case Enum_AcquisitionMode::PeakDetect:     return "PEAK";
    case Enum_AcquisitionMode::Average:        return "AVERage";
    case Enum_AcquisitionMode::HighResolution: return "HRESolution";
    case Enum_AcquisitionMode::Envelope:       return "ENVelope";
    default:                                   return "NORMal";
    }
}

QString CScpiCommandBuilder::measurementToken(Enum_MeasurementType in_enumType)
{
    switch (in_enumType)
    {
    case Enum_MeasurementType::Vpp:        return "VPP";
    case Enum_MeasurementType::Vamplitude: return "VAMPlitude";
    case Enum_MeasurementType::Vrms:       return "VRMS";
    case Enum_MeasurementType::Vmax:       return "VMAX";
    case Enum_MeasurementType::Vmin:       return "VMIN";
    case Enum_MeasurementType::Vaverage:   return "VAVerage";
    case Enum_MeasurementType::Frequency:  return "FREQuency";
    case Enum_MeasurementType::Period:     return "PERiod";
    case Enum_MeasurementType::RiseTime:   return "RISetime";
    case Enum_MeasurementType::FallTime:   return "FALLtime";
    case Enum_MeasurementType::DutyCycle:  return "DUTYcycle";
    case Enum_MeasurementType::PulseWidth: return "PWIDth";
    case Enum_MeasurementType::Overshoot:  return "OVERshoot";
    default:                               return "VPP";
    }
}

ScopeError CScpiCommandBuilder::validateChannel(const S_ScopeCapabilities& in_sCaps, U32BIT in_u32Channel)
{
    if (in_u32Channel < 1 || in_u32Channel > in_sCaps.m_u32NumberOfChannels)
    {
        return ScopeError(ScopeErrorCode::INVALID_CHANNEL,
                          QString("Channel %1 out of range (1..%2)")
                              .arg(in_u32Channel).arg(in_sCaps.m_u32NumberOfChannels));
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CScpiCommandBuilder::validateVerticalScale(const S_ScopeCapabilities& in_sCaps,
                                                      U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv)
{
    ScopeError chErr = validateChannel(in_sCaps, in_u32Channel);
    if (!chErr.isSuccess()) return chErr;

    const S_ChannelCapabilities ch = in_sCaps.channel(in_u32Channel);
    if (in_dVoltsPerDiv < ch.m_dMinVoltsPerDiv || in_dVoltsPerDiv > ch.m_dMaxVoltsPerDiv)
    {
        return ScopeError(ScopeErrorCode::INVALID_VOLTS_PER_DIV,
                          QString("%1 V/div outside [%2, %3]")
                              .arg(in_dVoltsPerDiv)
                              .arg(ch.m_dMinVoltsPerDiv)
                              .arg(ch.m_dMaxVoltsPerDiv));
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CScpiCommandBuilder::validateTimebaseScale(const S_ScopeCapabilities& in_sCaps,
                                                      FDOUBLE in_dSecondsPerDiv)
{
    if (in_dSecondsPerDiv < in_sCaps.m_dMinSecondsPerDiv ||
        in_dSecondsPerDiv > in_sCaps.m_dMaxSecondsPerDiv)
    {
        return ScopeError(ScopeErrorCode::INVALID_SECONDS_PER_DIV,
                          QString("%1 s/div outside [%2, %3]")
                              .arg(in_dSecondsPerDiv)
                              .arg(in_sCaps.m_dMinSecondsPerDiv)
                              .arg(in_sCaps.m_dMaxSecondsPerDiv));
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CScpiCommandBuilder::validateCoupling(const S_ScopeCapabilities& in_sCaps,
                                                 U32BIT in_u32Channel, Enum_Coupling in_enumCoupling)
{
    ScopeError chErr = validateChannel(in_sCaps, in_u32Channel);
    if (!chErr.isSuccess()) return chErr;

    const S_ChannelCapabilities ch = in_sCaps.channel(in_u32Channel);
    if (in_enumCoupling == Enum_Coupling::AC && !ch.m_bHasACCoupling)
    {
        return ScopeError(ScopeErrorCode::INVALID_COUPLING, "AC coupling not supported on this channel");
    }
    if (in_enumCoupling == Enum_Coupling::GND && !ch.m_bHasGNDCoupling)
    {
        return ScopeError(ScopeErrorCode::INVALID_COUPLING, "GND coupling not supported on this channel");
    }
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CScpiCommandBuilder::validateAcquireMode(const S_ScopeCapabilities& in_sCaps,
                                                    Enum_AcquisitionMode in_enumMode)
{
    if (in_sCaps.QlistAcquisitionModes.isEmpty() ||
        in_sCaps.QlistAcquisitionModes.contains(in_enumMode))
    {
        return ScopeError(ScopeErrorCode::SUCCESS);
    }
    return ScopeError(ScopeErrorCode::INVALID_ACQUIRE_MODE, "Acquisition mode not supported by model");
}
