// VisaHelper.h
//
// Transport-only helper carried over almost verbatim from the ELoad_R2
// reference. Builds a VISA resource string from an S_ConnectionConfig for every
// supported protocol. It is instrument-agnostic (no scope/PSU specifics), which
// is exactly why it can be reused unchanged. Include this in one translation
// unit per plugin (the reference includes it from the plugin .cpp).

#ifndef VISAHELPER_H
#define VISAHELPER_H

#include <QString>
#include <QDebug>
#include "ScopeTypes.h"

inline QString S_ConnectionConfig::toVisaResourceString() const
{
    QString StrResource;

    // An explicit resource string overrides everything else.
    QString primaryResource = QString::fromLocal8Bit(m_szResourceString);
    if (!primaryResource.isEmpty())
    {
        return primaryResource;
    }

    QString StrPortName        = QString::fromLocal8Bit(m_szPortName);
    QString StrIpAddress       = QString::fromLocal8Bit(m_szIpAddress);
    QString StrUsbVendorId     = QString::fromLocal8Bit(m_szUsbVendorId);
    QString StrUsbProductId    = QString::fromLocal8Bit(m_szUsbProductId);
    QString StrUsbSerialNumber = QString::fromLocal8Bit(m_szUsbSerialNumber);

    switch (m_enumProtocol)
    {
    case Enum_CommunicationProtocol::RS232:
    case Enum_CommunicationProtocol::RS485:
        // ASRL[board]::INSTR
        if (StrPortName.startsWith("/dev/"))
        {
            StrResource = QString("ASRL%1::INSTR").arg(StrPortName);
        }
        else if (StrPortName.toUpper().startsWith("COM"))
        {
            QString numStr = StrPortName.mid(3);
            StrResource = QString("ASRL%1::INSTR").arg(numStr);
        }
        else
        {
            StrResource = QString("ASRL%1::INSTR").arg(StrPortName);
        }
        break;

    case Enum_CommunicationProtocol::GPIB:
        // GPIB[board]::primary address::INSTR
        StrResource = QString("GPIB%1::%2::INSTR")
                          .arg(m_u32GpibBoard)
                          .arg(m_u32GpibAddress);
        break;

    case Enum_CommunicationProtocol::USB:
        // USB[board]::vendor::product::serial::INSTR  (USB-TMC)
        if (!StrUsbVendorId.isEmpty() && !StrUsbProductId.isEmpty())
        {
            if (!StrUsbSerialNumber.isEmpty())
            {
                StrResource = QString("USB0::%1::%2::%3::INSTR")
                                  .arg(StrUsbVendorId)
                                  .arg(StrUsbProductId)
                                  .arg(StrUsbSerialNumber);
            }
            else
            {
                StrResource = QString("USB0::%1::%2::INSTR")
                                  .arg(StrUsbVendorId)
                                  .arg(StrUsbProductId);
            }
        }
        else
        {
            StrResource = "USB0::INSTR";
        }
        break;

    case Enum_CommunicationProtocol::TCPIP:
    case Enum_CommunicationProtocol::ETHERNET:
    case Enum_CommunicationProtocol::LXI:
        // Port 5025 => raw SCPI socket; standard LXI/VXI-11 uses INSTR.
        if (m_u32Port != 5025)
        {
            StrResource = QString("TCPIP0::%1::%2::SOCKET")
                              .arg(StrIpAddress)
                              .arg(m_u32Port);
        }
        else
        {
            StrResource = QString("TCPIP0::%1::inst0::INSTR")
                              .arg(StrIpAddress);
        }
        break;

    case Enum_CommunicationProtocol::VXI11:
        StrResource = QString("TCPIP0::%1::inst0::INSTR").arg(StrIpAddress);
        break;

    default:
        StrResource = "INSTR";
        break;
    }

    return StrResource;
}

#endif // VISAHELPER_H
