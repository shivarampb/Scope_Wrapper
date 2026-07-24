// CScpiCommandBuilder.h
//
// Shared helper that composes SCPI command strings from templates + runtime
// parameters and validates parameters against instrument capabilities BEFORE
// transmit. Plugins use this so commands are built, not free-typed (the
// reference formats numerics inline, e.g. QString(":CUR%1;").arg(v,0,'f',3));
// this centralises that discipline and the pre-flight range checks.

#ifndef CSCPICOMMANDBUILDER_H
#define CSCPICOMMANDBUILDER_H

#include <QString>
#include "dp_types.h"
#include "ScopeError.h"
#include "ScopeTypes.h"

class CScpiCommandBuilder {
public:
    // ---- Numeric formatting ----
    // Engineering/scientific format accepted by every scope SCPI parser.
    static QString formatValue(FDOUBLE in_dValue, int in_iPrecision = 6);

    // ---- Template filling ----
    // fill(":CHANnel%1:SCALe %2", chan, formatValue(v)) style helpers.
    static QString fill(const QString& in_kTemplate, U32BIT in_u32Arg1);
    static QString fill(const QString& in_kTemplate, U32BIT in_u32Arg1, const QString& in_kArg2);
    static QString fill(const QString& in_kTemplate, const QString& in_kArg1);

    // ---- Enum -> SCPI token mapping ----
    static QString couplingToken(Enum_Coupling in_enumCoupling);
    static QString slopeToken(Enum_TriggerSlope in_enumSlope);
    static QString triggerModeToken(Enum_TriggerMode in_enumMode);
    static QString acquireModeToken(Enum_AcquisitionMode in_enumMode);
    static QString measurementToken(Enum_MeasurementType in_enumType);

    // ---- Parameter validation against capabilities ----
    // Each returns SUCCESS or the specific ScopeErrorCode-bearing ScopeError,
    // mirroring the reference's pre-transmit guards (INVALID_CHANNEL, range).
    static ScopeError validateChannel(const S_ScopeCapabilities& in_sCaps, U32BIT in_u32Channel);
    static ScopeError validateVerticalScale(const S_ScopeCapabilities& in_sCaps,
                                            U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv);
    static ScopeError validateTimebaseScale(const S_ScopeCapabilities& in_sCaps,
                                            FDOUBLE in_dSecondsPerDiv);
    static ScopeError validateCoupling(const S_ScopeCapabilities& in_sCaps,
                                       U32BIT in_u32Channel, Enum_Coupling in_enumCoupling);
    static ScopeError validateAcquireMode(const S_ScopeCapabilities& in_sCaps,
                                          Enum_AcquisitionMode in_enumMode);
};

#endif // CSCPICOMMANDBUILDER_H
