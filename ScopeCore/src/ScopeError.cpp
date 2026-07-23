// ScopeError.cpp
//
// Implementation of the scope error/status value types. Mirrors the structure
// of the ELoad_R2 `PowerSupplyError.cpp` (constructors, code->string table,
// status flag decoding, aggregate toString()).

#include "ScopeError.h"

ScopeError::ScopeError()
    : m_code(ScopeErrorCode::SUCCESS), m_description("Success")
{
}

ScopeError::ScopeError(ScopeErrorCode in_code, const QString& in_StrDescription)
    : m_code(in_code), m_description(in_StrDescription)
{
    if (m_description.isEmpty())
    {
        m_description = errorCodeToString(in_code);
    }
}

QString ScopeError::toString() const
{
    return QString("[%1] %2").arg(static_cast<int>(m_code)).arg(m_description);
}

QString ScopeError::errorCodeToString(ScopeErrorCode in_eCode)
{
    switch (in_eCode)
    {
    case ScopeErrorCode::SUCCESS:                 return "Success";
    case ScopeErrorCode::CONNECTION_FAILED:       return "Connection failed";
    case ScopeErrorCode::DISCONNECTION_FAILED:    return "Disconnection failed";
    case ScopeErrorCode::ALREADY_CONNECTED:       return "Already connected";
    case ScopeErrorCode::NOT_CONNECTED:           return "Not connected";
    case ScopeErrorCode::COMMUNICATION_TIMEOUT:   return "Communication timeout";
    case ScopeErrorCode::COMMUNICATION_ERROR:     return "Communication error";
    case ScopeErrorCode::INVALID_RESPONSE:        return "Invalid response from device";
    case ScopeErrorCode::COMMAND_FAILED:          return "Command execution failed";
    case ScopeErrorCode::SET_VERTICAL_FAILED:     return "Failed to set vertical settings";
    case ScopeErrorCode::SET_TIMEBASE_FAILED:     return "Failed to set timebase";
    case ScopeErrorCode::SET_TRIGGER_FAILED:      return "Failed to set trigger";
    case ScopeErrorCode::SET_ACQUIRE_FAILED:      return "Failed to set acquisition";
    case ScopeErrorCode::READ_MEASUREMENT_FAILED: return "Failed to read measurement";
    case ScopeErrorCode::WAVEFORM_TRANSFER_FAILED:return "Waveform transfer failed";
    case ScopeErrorCode::AUTOSET_FAILED:          return "Auto-setup failed";
    case ScopeErrorCode::ILLEGAL_COMMAND_OR_QUERY:return "Illegal command or query";
    case ScopeErrorCode::MISSING_PARAMETER:       return "Missing parameter";
    case ScopeErrorCode::ILLEGAL_PARAMETER:       return "Illegal parameter";
    case ScopeErrorCode::SETTING_CONFLICT:        return "Settings conflict";
    case ScopeErrorCode::SETTING_OUT_OF_RANGE:    return "Setting out of range";
    case ScopeErrorCode::PARAMETER_OUT_OF_RANGE:  return "Parameter out of range";
    case ScopeErrorCode::INVALID_VOLTS_PER_DIV:   return "Invalid volts/div value";
    case ScopeErrorCode::INVALID_SECONDS_PER_DIV: return "Invalid seconds/div value";
    case ScopeErrorCode::INVALID_TRIGGER_LEVEL:   return "Invalid trigger level";
    case ScopeErrorCode::INVALID_COUPLING:        return "Invalid coupling";
    case ScopeErrorCode::INVALID_SCOPE_NUMBER:    return "Invalid scope number";
    case ScopeErrorCode::INVALID_ADDRESS:         return "Invalid device address";
    case ScopeErrorCode::INVALID_CHANNEL:         return "Invalid channel number";
    case ScopeErrorCode::INVALID_SOURCE:          return "Invalid waveform source";
    case ScopeErrorCode::INVALID_MEASUREMENT:     return "Invalid measurement type";
    case ScopeErrorCode::INVALID_ACQUIRE_MODE:    return "Invalid acquisition mode";
    case ScopeErrorCode::DEVICE_ERROR:            return "Device error";
    case ScopeErrorCode::ACQUISITION_TIMEOUT:     return "Acquisition timeout";
    case ScopeErrorCode::NO_TRIGGER:              return "No trigger detected";
    case ScopeErrorCode::OVERLOAD:                return "Input overload";
    case ScopeErrorCode::NOT_TRIGGERED:           return "Not triggered";
    case ScopeErrorCode::ACQUISITION_STOPPED:     return "Acquisition stopped";
    case ScopeErrorCode::PLUGIN_ERROR:            return "Plugin error";
    case ScopeErrorCode::PLUGIN_NOT_FOUND:        return "Plugin not found";
    case ScopeErrorCode::PLUGIN_LOAD_FAILED:      return "Failed to load plugin";
    case ScopeErrorCode::PLUGIN_VERSION_MISMATCH: return "Plugin version mismatch";
    case ScopeErrorCode::UNKNOWN_ERROR:           return "Unknown error";
    default:                                      return "Undefined error";
    }
}

QString ScopeError::deviceStatusToString(DeviceStatus in_status)
{
    QStringList states;

    if (in_status & DeviceStatusFlag::Triggered)        states << "Triggered";
    if (in_status & DeviceStatusFlag::Armed)            states << "Armed";
    if (in_status & DeviceStatusFlag::AutoTriggered)    states << "Auto-Triggered";
    if (in_status & DeviceStatusFlag::Running)          states << "Running";
    if (in_status & DeviceStatusFlag::Stopped)          states << "Stopped";
    if (in_status & DeviceStatusFlag::ClippingPositive) states << "Clipping (+)";
    if (in_status & DeviceStatusFlag::ClippingNegative) states << "Clipping (-)";
    if (in_status & DeviceStatusFlag::Overload)         states << "Overload";
    if (in_status & DeviceStatusFlag::CalibrationError) states << "Calibration Error";
    if (in_status & DeviceStatusFlag::MaskTestFail)     states << "Mask Test Fail";
    if (in_status & DeviceStatusFlag::HardwareError)    states << "Hardware Error";
    if (in_status & DeviceStatusFlag::CommandError)     states << "Command Error";
    if (in_status & DeviceStatusFlag::ExecutionError)   states << "Execution Error";
    if (in_status & DeviceStatusFlag::QueryError)       states << "Query Error";
    if (in_status & DeviceStatusFlag::PowerOnReset)     states << "Power-On Reset";

    if (states.isEmpty())
        return "No Error";

    return states.join(", ");
}

QString ScopeError::operatingStateToString(Enum_OperatingState in_enumState)
{
    switch (in_enumState)
    {
    case Enum_OperatingState::m_enumRun:     return "Run";
    case Enum_OperatingState::m_enumStop:    return "Stop";
    case Enum_OperatingState::m_enumSingle:  return "Single";
    case Enum_OperatingState::m_enumWaiting: return "Waiting for Trigger";
    case Enum_OperatingState::m_enumUnknown:
    default:                                 return "Unknown";
    }
}

QString S_DeviceErrorStatus::toString() const
{
    QStringList qstrlstParts;

    qstrlstParts << "State: "  + ScopeError::operatingStateToString(m_enumState);
    qstrlstParts << "Status: " + ScopeError::deviceStatusToString(m_statusFlags);

    if (!m_StrErrorMessage.isEmpty())
    {
        qstrlstParts << "Message: " + m_StrErrorMessage;
    }
    if (m_iStandardEventStatus != 0)
    {
        qstrlstParts << QString("ESR: 0x%1").arg(m_iStandardEventStatus, 0, 16);
    }
    if (m_iQuestionableStatus != 0)
    {
        qstrlstParts << QString("QUES: 0x%1").arg(m_iQuestionableStatus, 0, 16);
    }
    if (m_iOperationStatus != 0)
    {
        qstrlstParts << QString("OPER: 0x%1").arg(m_iOperationStatus, 0, 16);
    }

    return qstrlstParts.join(" | ");
}
