// ScopeFamilies.h
//
// Per-vendor SCPI dialect factories + family base classes. Each factory returns
// an S_ScpiDialect preset for a vendor's command set (the "scpi_map made code").
// Family bases override CVisaScopePlugin virtuals only where a vendor's
// waveform/measurement/trigger transfer differs from the generic default.
//
// Concrete model plugins derive from the matching family base (or directly from
// CVisaScopePlugin for Keysight) and just set m_dialect/m_caps/m_info.

#ifndef SCOPEFAMILIES_H
#define SCOPEFAMILIES_H

#include "CVisaScopePlugin.h"

// ---- Construction helpers (keep concrete plugins tiny) ----
S_PluginInfo makeScopePluginInfo(const char* in_szName, const char* in_szVersion,
                                 const char* in_szManufacturer, const char* in_szModel,
                                 const char* in_szDescription, const QStringList& in_protocols);

S_ScopeCapabilities makeScopeCaps(U32BIT in_u32Channels, FDOUBLE in_dBandwidthHz,
                                  FDOUBLE in_dMaxSampleRate, U64BIT in_u64MemoryDepth,
                                  FDOUBLE in_dMinSecPerDiv, FDOUBLE in_dMaxSecPerDiv,
                                  FDOUBLE in_dMinVoltsPerDiv, FDOUBLE in_dMaxVoltsPerDiv,
                                  bool in_bHasGndCoupling, bool in_bIsMSO = false,
                                  U32BIT in_u32DigitalChannels = 0);

// ---- Dialect factories ----
S_ScpiDialect keysightInfiniiVisionDialect(); // DSOX/DSO/MSO X-Series (Keysight-style waveform)
S_ScpiDialect keysightInfiniiumDialect();     // Infiniium S-Series
S_ScpiDialect tektronixDialect();             // TDS/MDO series
S_ScpiDialect rohdeSchwarzDialect();          // RTM/RTO series
S_ScpiDialect leCroyDialect();                // WaveSurfer / classic LeCroy

// ---- Tektronix family: MEASUrement:IMMed measure + WFMOutpre/CURVe waveform ----
class CTektronixScopeBase : public CVisaScopePlugin {
public:
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) override;
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform) override;
};

// ---- Rohde & Schwarz family: FORMat REAL + CHANnel:DATA? float block,
//      and slot-based MEASurement<n> ----
class CRohdeSchwarzScopeBase : public CVisaScopePlugin {
public:
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) override;
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform) override;
};

// ---- LeCroy family: divergent command set (VDIV/TDIV/PAVA/INSPECT) ----
class CLeCroyScopeBase : public CVisaScopePlugin {
public:
    ScopeError setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger) override;
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) override;
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform) override;
};

#endif // SCOPEFAMILIES_H
