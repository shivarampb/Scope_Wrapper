// KeysightDSOX2012APlugin.h
//
// Concrete scope plugin for the Keysight/Agilent InfiniiVision DSOX2012A
// (100 MHz, 2 analog channels). Structural analogue of the ELoad_R2
// TDKLambdaZUP36_6Plugin: QObject + interface, Q_PLUGIN_METADATA, per-instance
// VISA session map, sendCommand()/buildCommand()/parse<Vendor>Error()/
// visaErrorToScope(), and hardcoded getCapabilities()/getPluginInfo().
//
// SCPI dialect: Keysight InfiniiVision 2000 X-Series Programmer's Guide.

#ifndef KEYSIGHTDSOX2012APLUGIN_H
#define KEYSIGHTDSOX2012APLUGIN_H

#include <QObject>
#include <QMap>
#include <visa.h>
#include "IScopePlugin.h"

class CKeysightDSOX2012APlugin : public QObject, public CIScopePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ScopePlugin_iid)
    Q_INTERFACES(CIScopePlugin)

public:
    CKeysightDSOX2012APlugin();
    virtual ~CKeysightDSOX2012APlugin();

    // ---- Plugin information ----
    S_PluginInfo        getPluginInfo() const override;
    S_ScopeCapabilities getCapabilities() const override;

    // ---- Connection management ----
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

    // ---- Measurements ----
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) override;

    // ---- Waveform ----
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform) override;

    // ---- Status / error ----
    ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus) override;
    ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber) override;
    ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage) override;
    ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;
    ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) override;

private:
    struct S_DeviceInstance {
        ViSession          m_vi;
        ViSession          m_defaultRM;
        S_ConnectionConfig m_sConfig;
        bool               m_bConnected;

        S_DeviceInstance() : m_vi(VI_NULL), m_defaultRM(VI_NULL), m_bConnected(false) {}
    };

    QMap<U32BIT, S_DeviceInstance> m_devices; // key = scopeNumber - 1

    // ---- Transport helpers (mirror the reference) ----
    QString    buildCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd) const;
    ScopeError sendCommand(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd);
    ScopeError sendQuery(U32BIT in_u32ScopeNumber, const QString& in_kqstrCmd, QString& out_qstrResponse);
    ScopeError readBinaryBlock(U32BIT in_u32ScopeNumber, QByteArray& out_baData);
    bool       validateResponse(const QString& in_kStrResponse) const;
    ScopeError visaErrorToScope(ViStatus in_ViStatus, const QString& in_kstrContext = "") const;
    ScopeError drainSystemError(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus);

    // ---- SCPI token helpers specific to this dialect ----
    QString    channelSource(U32BIT in_u32Channel) const; // e.g. "CHANnel1"
};

#endif // KEYSIGHTDSOX2012APLUGIN_H
