// ScopeError.h
//
// Error / status model for the scope plugin system. Direct analogue of the
// ELoad_R2 `PowerSupplyError.h`:
//   ScopeErrorCode      - banded error enum (reuses the reference's ranges:
//                         connection 1000s, command 2000s, param 3000s,
//                         device 4000s, plugin 5000s) with scope-specific codes
//   DeviceStatusFlag    - Q_DECLARE_FLAGS bitmask of instrument conditions
//   Enum_OperatingState - scope run state (replaces PSU Enum_OperatingMode)
//   S_DeviceErrorStatus - aggregate status incl. IEEE-488.2 / SCPI registers
//   ScopeError          - value type returned from every API call (no throws)

#ifndef SCOPEERROR_H
#define SCOPEERROR_H

#include <QString>
#include <QStringList>
#include <QFlags>
#include <QDebug>

enum class ScopeErrorCode {
    SUCCESS = 0,

    // --- Connection / communication (1000s) ---
    CONNECTION_FAILED       = 1000,
    DISCONNECTION_FAILED    = 1001,
    ALREADY_CONNECTED       = 1002,
    NOT_CONNECTED           = 1003,
    COMMUNICATION_TIMEOUT   = 1004,
    COMMUNICATION_ERROR     = 1005,
    INVALID_RESPONSE        = 1006,

    // --- Command / operation (2000s) ---
    COMMAND_FAILED          = 2000,
    SET_VERTICAL_FAILED     = 2001,
    SET_TIMEBASE_FAILED     = 2002,
    SET_TRIGGER_FAILED      = 2003,
    SET_ACQUIRE_FAILED      = 2004,
    READ_MEASUREMENT_FAILED = 2005,
    WAVEFORM_TRANSFER_FAILED= 2006,
    AUTOSET_FAILED          = 2007,
    ILLEGAL_COMMAND_OR_QUERY= 2009, // maps device "command error"
    MISSING_PARAMETER       = 2010,
    ILLEGAL_PARAMETER       = 2011,
    SETTING_CONFLICT        = 2012,
    SETTING_OUT_OF_RANGE    = 2013,
    SET_MEMORY_FAILED       = 2014,
    SET_SAMPLERATE_FAILED   = 2015,
    SCREENSHOT_FAILED       = 2016,
    SAVE_RECALL_FAILED      = 2017,
    NOT_SUPPORTED           = 2099, // feature not supported by this model

    // --- Parameter validation (3000s) ---
    PARAMETER_OUT_OF_RANGE  = 3000,
    INVALID_VOLTS_PER_DIV   = 3001,
    INVALID_SECONDS_PER_DIV = 3002,
    INVALID_TRIGGER_LEVEL   = 3003,
    INVALID_COUPLING        = 3004,
    INVALID_SCOPE_NUMBER    = 3005,
    INVALID_ADDRESS         = 3006,
    INVALID_CHANNEL         = 3007,
    INVALID_SOURCE          = 3008,
    INVALID_MEASUREMENT     = 3009,
    INVALID_ACQUIRE_MODE    = 3010,

    // --- Device / acquisition state (4000s) ---
    DEVICE_ERROR            = 4000,
    ACQUISITION_TIMEOUT     = 4001,
    NO_TRIGGER              = 4002,
    OVERLOAD                = 4003,
    NOT_TRIGGERED           = 4004,
    ACQUISITION_STOPPED     = 4005,

    // --- Plugin lifecycle (5000s) ---
    PLUGIN_ERROR            = 5000,
    PLUGIN_NOT_FOUND        = 5001,
    PLUGIN_LOAD_FAILED      = 5002,
    PLUGIN_VERSION_MISMATCH = 5003,

    UNKNOWN_ERROR           = 9999
};

// Instrument condition flags (scope-flavoured DeviceStatusFlag).
enum class DeviceStatusFlag {
    NoError          = 0x0000,
    Triggered        = 0x0001,
    Armed            = 0x0002,
    AutoTriggered    = 0x0004,
    Running          = 0x0008,
    Stopped          = 0x0010,
    ClippingPositive = 0x0020,
    ClippingNegative = 0x0040,
    Overload         = 0x0080,
    CalibrationError = 0x0100,
    MaskTestFail     = 0x0200,
    HardwareError    = 0x0800,
    CommandError     = 0x1000,
    ExecutionError   = 0x2000,
    QueryError       = 0x4000,
    PowerOnReset     = 0x8000
};
Q_DECLARE_FLAGS(DeviceStatus, DeviceStatusFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(DeviceStatus)

// Scope run state (replaces the PSU Enum_OperatingMode CV/CC/UR).
enum class Enum_OperatingState {
    m_enumRun,      // continuously acquiring
    m_enumStop,     // acquisition halted
    m_enumSingle,   // single-shot armed
    m_enumWaiting,  // armed, awaiting trigger
    m_enumUnknown
};

struct S_DeviceErrorStatus {
    DeviceStatus        m_statusFlags;
    Enum_OperatingState m_enumState;
    QString             m_StrErrorMessage;
    int                 m_iStandardEventStatus;  // *ESR?  (IEEE 488.2)
    int                 m_iQuestionableStatus;    // SCPI Questionable register
    int                 m_iOperationStatus;       // SCPI Operation register

    S_DeviceErrorStatus()
        : m_statusFlags(DeviceStatusFlag::NoError)
        , m_enumState(Enum_OperatingState::m_enumUnknown)
        , m_iStandardEventStatus(0)
        , m_iQuestionableStatus(0)
        , m_iOperationStatus(0)
    {}

    bool hasError() const
    {
        // Only genuine fault bits count as an error condition; run/trigger
        // state bits (Triggered/Running/etc.) are informational.
        const int kFaultMask =
            static_cast<int>(DeviceStatusFlag::Overload) |
            static_cast<int>(DeviceStatusFlag::CalibrationError) |
            static_cast<int>(DeviceStatusFlag::HardwareError) |
            static_cast<int>(DeviceStatusFlag::CommandError) |
            static_cast<int>(DeviceStatusFlag::ExecutionError) |
            static_cast<int>(DeviceStatusFlag::QueryError);
        return (m_statusFlags.operator int() & kFaultMask) != 0;
    }

    QString toString() const;
};

class ScopeError {
public:
    ScopeError();
    ScopeError(ScopeErrorCode in_code, const QString& in_StrDescription = "");

    ScopeErrorCode code() const { return m_code; }
    QString        description() const { return m_description; }
    QString        toString() const;
    bool           isSuccess() const { return m_code == ScopeErrorCode::SUCCESS; }

    static QString errorCodeToString(ScopeErrorCode in_eCode);
    static QString deviceStatusToString(DeviceStatus in_status);
    static QString operatingStateToString(Enum_OperatingState in_enumState);

private:
    ScopeErrorCode m_code;
    QString        m_description;
};

#endif // SCOPEERROR_H
