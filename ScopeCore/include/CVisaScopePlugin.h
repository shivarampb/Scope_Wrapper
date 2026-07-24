// CVisaScopePlugin.h
//
// Shared base for all VISA/SCPI scope plugins. Implements the CIScopePlugin
// interface generically, driven by an S_ScpiDialect the derived model fills in
// its constructor. Holds the per-instance VISA session map and the transport
// (the code the ELoad_R2 plugins each duplicated; here factored once).
//
// A concrete plugin does three things: (1) derive QObject + this base with
// Q_OBJECT/Q_PLUGIN_METADATA/Q_INTERFACES, (2) set m_dialect and m_caps and
// m_info in its constructor. Vendors whose waveform/measurement transfer does
// not match the generic (Keysight-style) default override the relevant virtual
// in a family base (see ScopeFamilies.h).
//
// This is NOT a QObject itself, so concrete plugins add QObject as their first
// base and there is a single QObject chain (standard Qt plugin pattern).

#ifndef CVISASCOPEPLUGIN_H
#define CVISASCOPEPLUGIN_H

#include <QMap>
#include <QByteArray>
#include <visa.h>
#include "IScopePlugin.h"
#include "ScopeDialect.h"

class CVisaScopePlugin : public CIScopePlugin {
public:
    CVisaScopePlugin();
    virtual ~CVisaScopePlugin();

    // ---- Plugin information (served from members set by the derived ctor) ----
    S_PluginInfo        getPluginInfo() const override { return m_info; }
    S_ScopeCapabilities getCapabilities() const override { return m_caps; }

    // ---- Connection ----
    ScopeError connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig) override;
    ScopeError disconnect(U32BIT in_u32ScopeNumber) override;
    bool       isConnected(U32BIT in_u32ScopeNumber) const override;
    ScopeError reset(U32BIT in_u32ScopeNumber) override;
    ScopeError autoSetup(U32BIT in_u32ScopeNumber) override;
    ScopeError checkOperationComplete(U32BIT in_u32ScopeNumber, bool& out_bComplete) override;

    // ---- Vertical ----
    ScopeError setChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bEnable) override;
    ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv) override;
    ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dOffsetVolts) override;
    ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Coupling in_enumCoupling) override;
    ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dAttenuation) override;
    ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bLimit) override;

    // ---- Horizontal ----
    ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv) override;
    ScopeError setHorizontalPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dDelaySeconds) override;
    ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U64BIT in_u64Points) override;
    ScopeError setSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE in_dSamplesPerSec) override;

    // ---- Trigger ----
    ScopeError setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger) override;
    ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, FDOUBLE in_dLevelVolts) override;
    ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_TriggerMode in_enumMode) override;

    // ---- Acquisition ----
    ScopeError setAcquireMode(U32BIT in_u32ScopeNumber, Enum_AcquisitionMode in_enumMode, U32BIT in_u32AverageCount) override;
    ScopeError run(U32BIT in_u32ScopeNumber) override;
    ScopeError stop(U32BIT in_u32ScopeNumber) override;
    ScopeError single(U32BIT in_u32ScopeNumber) override;
    ScopeError forceTrigger(U32BIT in_u32ScopeNumber) override;

    // ---- Measurement (generic ":MEASure:<T>? <src>"; overridable) ----
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) override;

    // ---- Waveform (Keysight-style default; overridable per family) ----
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform) override;

    // ---- Screenshot / setup memory / MSO digital ----
    ScopeError getScreenshot(U32BIT in_u32ScopeNumber, QByteArray& out_baImage) override;
    ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) override;
    ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) override;
    ScopeError setDigitalChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, bool in_bEnable) override;
    ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Group, FDOUBLE in_dThresholdVolts) override;

    // ---- Status / error ----
    ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus) override;
    ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber) override;
    ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage) override;
    ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;

protected:
    struct S_DeviceInstance {
        ViSession          m_vi;
        ViSession          m_defaultRM;
        S_ConnectionConfig m_sConfig;
        bool               m_bConnected;
        S_DeviceInstance() : m_vi(VI_NULL), m_defaultRM(VI_NULL), m_bConnected(false) {}
    };

    // Filled by the derived model's constructor.
    S_PluginInfo        m_info;
    S_ScopeCapabilities m_caps;
    S_ScpiDialect       m_dialect;

    QMap<U32BIT, S_DeviceInstance> m_devices; // key = scopeNumber - 1

    // ---- Transport primitives (shared) ----
    ScopeError sendCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd);
    ScopeError sendQuery(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd, QString& out_qstrResponse);
    ScopeError readBinaryBlock(U32BIT in_u32ScopeNumber, QByteArray& out_baData);
    ScopeError visaErrorToScope(ViStatus in_ViStatus, const QString& in_kstrContext = "") const;

    // Helper used by the generic setters.
    ScopeError applyTemplate(U32BIT in_u32ScopeNumber, const QString& in_kTemplateSourceValue,
                             U32BIT in_u32Channel, const QString& in_kValue);
};

#endif // CVISASCOPEPLUGIN_H
