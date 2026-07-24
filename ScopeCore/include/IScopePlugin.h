// IScopePlugin.h
//
// Abstract plugin interface for oscilloscopes. Direct analogue of the ELoad_R2
// `IPowerSupplyPlugin.h` (Qt plugin interface + Q_DECLARE_INTERFACE + IID).
// Each concrete scope family implements this; CScopeManager loads them
// dynamically via QPluginLoader.
//
// Convention (from the reference): every call returns a ScopeError value type;
// query results come back through out-parameters; the first argument is the
// 1-based instance handle (in_u32ScopeNumber).

#ifndef ISCOPEPLUGIN_H
#define ISCOPEPLUGIN_H

#include <QtPlugin>
#include <QByteArray>
#include "ScopeError.h"
#include "ScopeTypes.h"

#define ScopePlugin_iid "com.automation.ScopePlugin/1.0"

class CIScopePlugin {
public:
    virtual ~CIScopePlugin() {}

    // ---- Plugin information ----
    virtual S_PluginInfo         getPluginInfo() const = 0;
    virtual S_ScopeCapabilities  getCapabilities() const = 0;

    // ---- Connection management ----
    virtual ScopeError connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig) = 0;
    virtual ScopeError disconnect(U32BIT in_u32ScopeNumber) = 0;
    virtual bool       isConnected(U32BIT in_u32ScopeNumber) const = 0;
    virtual ScopeError reset(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError autoSetup(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError checkOperationComplete(U32BIT in_u32ScopeNumber, bool& out_bComplete) = 0;

    // ---- Vertical (per channel) ----
    virtual ScopeError setChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bEnable) = 0;
    virtual ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv) = 0;
    virtual ScopeError setVerticalOffset(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dOffsetVolts) = 0;
    virtual ScopeError setCoupling(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, Enum_Coupling in_enumCoupling) = 0;
    virtual ScopeError setProbeAttenuation(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dAttenuation) = 0;
    virtual ScopeError setBandwidthLimit(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, bool in_bLimit) = 0;

    // ---- Horizontal / timebase ----
    virtual ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv) = 0;
    virtual ScopeError setHorizontalPosition(U32BIT in_u32ScopeNumber, FDOUBLE in_dDelaySeconds) = 0;
    virtual ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U64BIT in_u64Points) = 0;
    virtual ScopeError setSampleRate(U32BIT in_u32ScopeNumber, FDOUBLE in_dSamplesPerSec) = 0;

    // ---- Trigger ----
    virtual ScopeError setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger) = 0;
    virtual ScopeError setTriggerLevel(U32BIT in_u32ScopeNumber, FDOUBLE in_dLevelVolts) = 0;
    virtual ScopeError setTriggerMode(U32BIT in_u32ScopeNumber, Enum_TriggerMode in_enumMode) = 0;

    // ---- Acquisition control ----
    virtual ScopeError setAcquireMode(U32BIT in_u32ScopeNumber, Enum_AcquisitionMode in_enumMode, U32BIT in_u32AverageCount) = 0;
    virtual ScopeError run(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError stop(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError single(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError forceTrigger(U32BIT in_u32ScopeNumber) = 0;

    // ---- Measurements ----
    virtual ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                               Enum_MeasurementType in_enumType, FDOUBLE& out_dValue) = 0;

    // ---- Waveform transfer ----
    virtual ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                       S_Waveform& out_sWaveform) = 0;

    // ---- Screenshot / setup memory / MSO digital ----
    virtual ScopeError getScreenshot(U32BIT in_u32ScopeNumber, QByteArray& out_baImage) = 0;
    virtual ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) = 0;
    virtual ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location) = 0;
    virtual ScopeError setDigitalChannelEnable(U32BIT in_u32ScopeNumber, U32BIT in_u32DigitalChannel, bool in_bEnable) = 0;
    virtual ScopeError setDigitalThreshold(U32BIT in_u32ScopeNumber, U32BIT in_u32Group, FDOUBLE in_dThresholdVolts) = 0;

    // ---- Status / error ----
    virtual ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus) = 0;
    virtual ScopeError clearErrorStatus(U32BIT in_u32ScopeNumber) = 0;
    virtual ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage) = 0;
    virtual ScopeError readStandardEventStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) = 0;
    virtual ScopeError readStatusByte(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) = 0;
    virtual ScopeError readQuestionableStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) = 0;
    virtual ScopeError readOperationStatus(U32BIT in_u32ScopeNumber, U32BIT& out_u32Status) = 0;
};

Q_DECLARE_INTERFACE(CIScopePlugin, ScopePlugin_iid)

#endif // ISCOPEPLUGIN_H
