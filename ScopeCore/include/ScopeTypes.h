// ScopeTypes.h
//
// Domain data structures for the scope plugin system. This is the oscilloscope
// analogue of the ELoad_R2 `PowerSupplyTypes.h`:
//   S_PluginInfo            - carried over as-is (plugin identity/versioning)
//   S_ConnectionConfig      - carried over (transport parameters + VISA string)
//   Enum_CommunicationProtocol - carried over
//   S_ScopeCapabilities     - replaces S_PowerSupplyCapabilities
//   S_ChannelCapabilities   - replaces the PSU channel caps
//   S_Waveform/Preamble     - NEW, scope-specific waveform transfer model
//   S_TriggerConfig         - NEW, trigger description
//
// POD structs use fixed-size char[] buffers (like the reference) so they remain
// ABI-stable across the dynamically loaded plugin boundary.

#ifndef SCOPETYPES_H
#define SCOPETYPES_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QVector>
#include <cstring>
#include "dp_types.h"

// ---- S_PluginInfo fixed buffer sizes (mirrors reference PLUGIN_INFO_*_SIZE) ----
#define PLUGIN_INFO_NAME_SIZE          64
#define PLUGIN_INFO_VERSION_SIZE       16
#define PLUGIN_INFO_MANUFACTURER_SIZE  64
#define PLUGIN_INFO_MODEL_NAME_SIZE    64
#define PLUGIN_INFO_DESCRIPTION_SIZE   256
#define PLUGIN_INFO_MIN_CORE_VERSION   16
#define PLUGIN_INFO_MAX_CORE_VERSION   16

// ---- Connection / scope domain enumerations ----

enum class Enum_CommunicationProtocol {
    RS232,
    RS485,
    GPIB,
    USB,      // USB-TMC
    TCPIP,
    ETHERNET,
    LXI,
    VXI11
};

enum class Enum_Coupling {
    DC,
    AC,
    GND,
    Unknown
};

enum class Enum_AcquisitionMode {
    Sample,
    PeakDetect,
    Average,
    HighResolution,
    Envelope,
    Unknown
};

enum class Enum_TriggerType {
    Edge,
    Pulse,
    Video,
    Pattern,
    Runt,
    Window,
    Unknown
};

enum class Enum_TriggerSlope {
    Rising,
    Falling,
    Either
};

enum class Enum_TriggerMode {
    Auto,
    Normal,
    Single
};

enum class Enum_MeasurementType {
    Vpp,
    Vamplitude,
    Vrms,
    Vmax,
    Vmin,
    Vaverage,
    Frequency,
    Period,
    RiseTime,
    FallTime,
    DutyCycle,
    PulseWidth,
    Overshoot,
    Unknown
};

enum class Enum_WaveformFormat {
    Byte,    // 8-bit  (:WAVeform:FORMat BYTE)
    Word,    // 16-bit (:WAVeform:FORMat WORD)
    Ascii    // comma-separated (:WAVeform:FORMat ASCii)
};

// ---- S_PluginInfo (carried over from reference, verbatim structure) ----

struct S_PluginInfo {
    char        m_szName[PLUGIN_INFO_NAME_SIZE];
    char        m_szVersion[PLUGIN_INFO_VERSION_SIZE];
    char        m_szManufacturer[PLUGIN_INFO_MANUFACTURER_SIZE];
    char        m_szModelName[PLUGIN_INFO_MODEL_NAME_SIZE];
    char        m_szDescription[PLUGIN_INFO_DESCRIPTION_SIZE];
    char        m_szMinCoreVersion[PLUGIN_INFO_MIN_CORE_VERSION];
    char        m_szMaxCoreVersion[PLUGIN_INFO_MAX_CORE_VERSION];
    QStringList m_StrlstSupportedProtocols;

    S_PluginInfo()
    {
        std::memset(m_szName, 0, sizeof(m_szName));
        std::memset(m_szVersion, 0, sizeof(m_szVersion));
        std::memset(m_szManufacturer, 0, sizeof(m_szManufacturer));
        std::memset(m_szModelName, 0, sizeof(m_szModelName));
        std::memset(m_szDescription, 0, sizeof(m_szDescription));
        std::memset(m_szMinCoreVersion, 0, sizeof(m_szMinCoreVersion));
        std::memset(m_szMaxCoreVersion, 0, sizeof(m_szMaxCoreVersion));
    }

    // Semantic-version compare of "MAJOR.MINOR.PATCH" strings; returns
    // in_kStrCoreVersion within [min, max]. Mirrors reference isCompatible().
    bool isCompatible(const QString& in_kStrCoreVersion) const;
};

// ---- Capability descriptors (scope analogue of PSU caps) ----

struct S_ChannelCapabilities {
    U32BIT  m_u32ChannelNumber;
    FDOUBLE m_dMinVoltsPerDiv;      // finest vertical scale (V/div)
    FDOUBLE m_dMaxVoltsPerDiv;      // coarsest vertical scale (V/div)
    FDOUBLE m_dMaxOffset;           // max |vertical offset| (V)
    FDOUBLE m_dMaxProbeAttenuation; // e.g. 1000x
    bool    m_bHasBandwidthLimit;
    bool    m_bHasACCoupling;
    bool    m_bHasGNDCoupling;
    bool    m_bIsDigital;           // MSO digital lane vs. analog channel

    S_ChannelCapabilities()
        : m_u32ChannelNumber(0)
        , m_dMinVoltsPerDiv(0.0)
        , m_dMaxVoltsPerDiv(0.0)
        , m_dMaxOffset(0.0)
        , m_dMaxProbeAttenuation(1.0)
        , m_bHasBandwidthLimit(false)
        , m_bHasACCoupling(true)
        , m_bHasGNDCoupling(false)
        , m_bIsDigital(false)
    {}
};

struct S_ScopeCapabilities {
    U32BIT  m_u32NumberOfChannels;   // analog channels
    U32BIT  m_u32DigitalChannels;    // MSO lanes (0 if not MSO)
    FDOUBLE m_dBandwidthHz;          // analog bandwidth
    FDOUBLE m_dMaxSampleRate;        // Sa/s
    U64BIT  m_u64MaxMemoryDepth;     // points
    FDOUBLE m_dMinSecondsPerDiv;     // fastest timebase (s/div)
    FDOUBLE m_dMaxSecondsPerDiv;     // slowest timebase (s/div)
    bool    m_bHasFFT;
    bool    m_bIsMSO;

    QList<S_ChannelCapabilities>  QlistChannels;
    QList<Enum_TriggerType>       QlistTriggerTypes;
    QList<Enum_AcquisitionMode>   QlistAcquisitionModes;

    S_ScopeCapabilities()
        : m_u32NumberOfChannels(0)
        , m_u32DigitalChannels(0)
        , m_dBandwidthHz(0.0)
        , m_dMaxSampleRate(0.0)
        , m_u64MaxMemoryDepth(0)
        , m_dMinSecondsPerDiv(0.0)
        , m_dMaxSecondsPerDiv(0.0)
        , m_bHasFFT(false)
        , m_bIsMSO(false)
    {}

    // Convenience lookup; returns nullptr-equivalent by value with channel 0.
    S_ChannelCapabilities channel(U32BIT in_u32ChannelNumber) const
    {
        for (const S_ChannelCapabilities& ch : QlistChannels)
        {
            if (ch.m_u32ChannelNumber == in_u32ChannelNumber)
            {
                return ch;
            }
        }
        return S_ChannelCapabilities();
    }
};

// ---- Trigger configuration ----

struct S_TriggerConfig {
    Enum_TriggerType  m_enumType;
    Enum_TriggerSlope m_enumSlope;
    Enum_TriggerMode  m_enumMode;
    U32BIT            m_u32Source;   // channel number (analog)
    FDOUBLE           m_dLevel;      // volts

    S_TriggerConfig()
        : m_enumType(Enum_TriggerType::Edge)
        , m_enumSlope(Enum_TriggerSlope::Rising)
        , m_enumMode(Enum_TriggerMode::Auto)
        , m_u32Source(1)
        , m_dLevel(0.0)
    {}
};

// ---- Waveform transfer model ----

struct S_WaveformPreamble {
    Enum_WaveformFormat m_enumFormat;
    U32BIT  m_u32Points;
    FDOUBLE m_dXIncrement;   // time between points (s)
    FDOUBLE m_dXOrigin;      // time of first point (s)
    FDOUBLE m_dXReference;   // reference index for X origin
    FDOUBLE m_dYIncrement;   // volts per code
    FDOUBLE m_dYOrigin;      // voltage at Y reference
    FDOUBLE m_dYReference;   // code that maps to Y origin

    S_WaveformPreamble()
        : m_enumFormat(Enum_WaveformFormat::Word)
        , m_u32Points(0)
        , m_dXIncrement(0.0)
        , m_dXOrigin(0.0)
        , m_dXReference(0.0)
        , m_dYIncrement(0.0)
        , m_dYOrigin(0.0)
        , m_dYReference(0.0)
    {}
};

struct S_Waveform {
    U32BIT             m_u32Source;   // channel the trace came from
    S_WaveformPreamble m_sPreamble;
    QVector<FDOUBLE>   m_dTime;        // seconds, size == m_u32Points
    QVector<FDOUBLE>   m_dVolts;       // volts,   size == m_u32Points

    S_Waveform() : m_u32Source(0) {}

    bool isEmpty() const { return m_dVolts.isEmpty(); }
};

// ---- S_ConnectionConfig (carried over from reference) ----

struct S_ConnectionConfig {
    Enum_CommunicationProtocol m_enumProtocol;

    char   m_szResourceString[128]; // optional explicit VISA resource override
    char   m_szPortName[32];        // e.g. "COM3" or "/dev/ttyUSB0"
    char   m_szIpAddress[64];
    char   m_szUsbVendorId[16];
    char   m_szUsbProductId[16];
    char   m_szUsbSerialNumber[64];

    U32BIT m_u32GpibBoard;
    U32BIT m_u32GpibAddress;
    U32BIT m_u32Port;               // TCP port (5025 => INSTR, else SOCKET)
    U32BIT m_u32Timeout;            // ms
    U32BIT m_u32BaudRate;
    U32BIT m_u32DeviceAddress;      // generic secondary address (unused for most scopes)

    S_ConnectionConfig()
        : m_enumProtocol(Enum_CommunicationProtocol::TCPIP)
        , m_u32GpibBoard(0)
        , m_u32GpibAddress(0)
        , m_u32Port(5025)
        , m_u32Timeout(5000)
        , m_u32BaudRate(9600)
        , m_u32DeviceAddress(0)
    {
        std::memset(m_szResourceString, 0, sizeof(m_szResourceString));
        std::memset(m_szPortName, 0, sizeof(m_szPortName));
        std::memset(m_szIpAddress, 0, sizeof(m_szIpAddress));
        std::memset(m_szUsbVendorId, 0, sizeof(m_szUsbVendorId));
        std::memset(m_szUsbProductId, 0, sizeof(m_szUsbProductId));
        std::memset(m_szUsbSerialNumber, 0, sizeof(m_szUsbSerialNumber));
    }

    // Defined in VisaHelper.h (kept out-of-line, exactly like the reference).
    QString toVisaResourceString() const;
};

#endif // SCOPETYPES_H
