// ScopeDialect.h
//
// S_ScpiDialect describes one vendor's SCPI command dialect as a table of
// string templates + token maps. It is the in-code realization of the
// per-model scpi_map.json from the development plan (Section 6): a concrete
// plugin fills a dialect + capabilities, and CVisaScopePlugin turns generic
// interface calls into wire commands through it. This keeps the identical VISA
// transport in one place (as the ELoad_R2 plugins each had, here shared) while
// each model still ships as its own dynamically-loaded plugin.
//
// Template placeholders use QString::arg positions:
//   %1 = channel source string (built from m_strChannelSourceFmt), where used
//   %2 = value / token
// Timebase/trigger templates that carry a single value use %1 for that value.

#ifndef SCOPEDIALECT_H
#define SCOPEDIALECT_H

#include <QString>
#include <QMap>
#include "ScopeTypes.h"

struct S_ScpiDialect {
    // Channel source naming: "CHANnel%1" (Keysight/R&S), "CH%1" (Tek), "C%1" (LeCroy)
    QString m_strChannelSourceFmt;

    // Common / system
    QString m_strIdn;            // "*IDN?"
    QString m_strReset;          // "*RST"
    QString m_strClear;          // "*CLS"
    QString m_strOpcQuery;       // "*OPC?"
    QString m_strAutoscale;      // ":AUToscale" / "AUTOSet EXECute"
    QString m_strSysError;       // ":SYSTem:ERRor?" / "EVMsg?"
    QString m_strEsrQuery;       // "*ESR?"
    QString m_strStbQuery;       // "*STB?"
    QString m_strQuesEvent;      // ":STATus:QUEStionable:EVENt?" (empty => unsupported)
    QString m_strOperEvent;      // ":STATus:OPERation:EVENt?"   (empty => unsupported)

    // Vertical (%1 = source, %2 = value/token)
    QString m_strChEnable;       // ":%1:DISPlay %2" / "SELect:%1 %2"
    QString m_strVScale;         // ":%1:SCALe %2"
    QString m_strVOffset;        // ":%1:OFFSet %2"
    QString m_strCoupling;       // ":%1:COUPling %2"
    QString m_strProbe;          // ":%1:PROBe %2"
    QString m_strBwLimit;        // ":%1:BWLimit %2" / "%1:BANdwidth %2"

    // Horizontal (%1 = value)
    QString m_strTimebaseScale;  // ":TIMebase:SCALe %1" / "HORizontal:SCAle %1"
    QString m_strTimebasePos;    // ":TIMebase:POSition %1" / "HORizontal:DELay:TIMe %1"

    // Trigger
    QString m_strTrigSource;     // ":TRIGger:EDGE:SOURce %1"
    QString m_strTrigSlope;      // ":TRIGger:EDGE:SLOPe %1"
    QString m_strTrigLevel;      // ":TRIGger:EDGE:LEVel %1"
    QString m_strTrigSweep;      // ":TRIGger:SWEep %1" / "TRIGger:A:MODe %1"

    // Acquisition
    QString m_strAcqType;        // ":ACQuire:TYPE %1" / "ACQuire:MODe %1"
    QString m_strAcqCount;       // ":ACQuire:COUNt %1" / "ACQuire:NUMAVg %1"
    QString m_strRun;            // ":RUN" / "ACQuire:STATE RUN"
    QString m_strStop;           // ":STOP" / "ACQuire:STATE STOP"
    QString m_strSingle;         // ":SINGle" / "ACQuire:STOPAfter SEQuence;:ACQuire:STATE ON"
    QString m_strForce;          // ":TRIGger:FORCe" / "TRIGger FORCe"

    // Measurement (%1 = type token, %2 = source). Vendors that don't fit this
    // shape override measure() in a family base instead.
    QString m_strMeasure;        // ":MEASure:%1? %2"

    // Token maps
    QString m_strOn, m_strOff;
    QString m_strBwOn, m_strBwOff;   // bandwidth-limit tokens (not always ON/OFF)
    QString m_strSlopeRising, m_strSlopeFalling, m_strSlopeEither;
    QString m_strSweepAuto, m_strSweepNormal;
    QString m_strCouplingDC, m_strCouplingAC, m_strCouplingGND;
    QMap<Enum_AcquisitionMode, QString>   m_mapAcqTokens;
    QMap<Enum_MeasurementType, QString>   m_mapMeasTokens;

    // Terminator appended to every write ("\n" for most).
    QString m_strTerminator;

    S_ScpiDialect() : m_strTerminator("\n") {}

    QString source(U32BIT in_u32Channel) const
    {
        return m_strChannelSourceFmt.arg(in_u32Channel);
    }
};

#endif // SCOPEDIALECT_H
