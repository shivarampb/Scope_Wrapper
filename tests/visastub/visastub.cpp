// visastub.cpp — in-process SCPI oscilloscope simulator behind the VISA API.
//
// Generates a deterministic ramp waveform (volts[i] = 0.1*i, 10 points,
// dt = 1 ns) in each vendor's transfer format so plugin waveform code can be
// exercised end-to-end. Records every written command for assertion by tests.

#include "visa.h"
#include "visastub_api.h"
#include <string>
#include <vector>
#include <deque>
#include <cstring>
#include <cstdint>

namespace {

const int    kPoints = 10;
const double kDt     = 1e-9;
double sampleVolts(int i) { return 0.1 * static_cast<double>(i); }

std::vector<std::string> g_commands;   // command log
std::deque<std::string>  g_responses;  // pending read payloads (may be binary)
std::string              g_readBuf;    // current payload being drained
ViSession                g_nextSession = 100;

bool contains(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }

std::string ieeeBlock(const std::string& payload)
{
    const std::string len = std::to_string(payload.size());
    return "#" + std::to_string(len.size()) + len + payload;
}

std::string keysightWordBlock()   // unsigned 16-bit little-endian
{
    std::string b;
    for (int i = 0; i < kPoints; ++i) {
        uint16_t raw = static_cast<uint16_t>(32768 + i * 100); // volts=(raw-32768)*0.001=0.1*i
        b.push_back(static_cast<char>(raw & 0xFF));
        b.push_back(static_cast<char>((raw >> 8) & 0xFF));
    }
    return ieeeBlock(b) + "\n";
}

std::string tektronixCurveBlock() // signed 16-bit big-endian
{
    std::string b;
    for (int i = 0; i < kPoints; ++i) {
        int16_t raw = static_cast<int16_t>(i * 100); // volts=(raw-0)*0.001=0.1*i
        b.push_back(static_cast<char>((raw >> 8) & 0xFF));
        b.push_back(static_cast<char>(raw & 0xFF));
    }
    return ieeeBlock(b) + "\n";
}

std::string rohdeFloatBlock()     // 32-bit float little-endian volts
{
    std::string b;
    for (int i = 0; i < kPoints; ++i) {
        float v = static_cast<float>(sampleVolts(i));
        char tmp[4]; std::memcpy(tmp, &v, 4);
        b.append(tmp, 4);
    }
    return ieeeBlock(b) + "\n";
}

std::string lecroySimple()
{
    std::string vals;
    for (int i = 0; i < kPoints; ++i) vals += std::to_string(sampleVolts(i)) + " ";
    return "C1:INSP \"" + vals + "\"\n";
}

std::string screenshotBlock()   // tiny fake image payload as a definite block
{
    std::string png(16, '\0');
    png[0] = char(0x89); png[1] = 'P'; png[2] = 'N'; png[3] = 'G';
    return ieeeBlock(png) + "\n";
}

// Compute a response for a query command.
std::string responseFor(const std::string& cmd)
{
    if (contains(cmd, "*IDN?"))   return "KEYSIGHT TECHNOLOGIES,DSO-X 2012A,MY51330623,07.30\n";
    if (contains(cmd, "*OPC?"))   return "1\n";

    // Screenshot (Keysight :DISPlay:DATA?, LeCroy SCDP)
    if (contains(cmd, "DISPlay:DATA?")) return screenshotBlock();
    if (cmd.rfind("SCDP", 0) == 0)      return screenshotBlock();
    if (contains(cmd, "*ESR?"))   return "0\n";
    if (contains(cmd, "*STB?"))   return "0\n";

    // Error queues (all drain to "no error").
    if (contains(cmd, "SYSTem:ERRor") || contains(cmd, "EVMsg?") || cmd == "CMR?")
        return "0,\"No error\"\n";
    if (contains(cmd, "QUEStionable") || contains(cmd, "OPERation:EVEN")) return "0\n";

    // Keysight waveform
    if (contains(cmd, "WAVeform:PREamble?"))
        return "+1,+0,+10,+1,+1.0E-09,+0.0,+0,+1.0E-03,+0.0,+3.2768E+04\n";
    if (contains(cmd, "WAVeform:DATA?"))  return keysightWordBlock();

    // Tektronix waveform preamble pieces + curve
    if (contains(cmd, "WFMOutpre:XINcr?")) return "1.0E-9\n";
    if (contains(cmd, "WFMOutpre:XZEro?")) return "0.0\n";
    if (contains(cmd, "WFMOutpre:YMUlt?")) return "1.0E-3\n";
    if (contains(cmd, "WFMOutpre:YOFf?"))  return "0.0\n";
    if (contains(cmd, "WFMOutpre:YZEro?")) return "0.0\n";
    if (contains(cmd, "WFMOutpre:NR_Pt?")) return "10\n";
    if (contains(cmd, "CURVe?"))           return tektronixCurveBlock();

    // Tektronix measurement
    if (contains(cmd, "MEASUrement:IMMed:VALue?")) return "1.0E0\n";

    // Rohde & Schwarz waveform
    if (contains(cmd, "DATA:HEADer?"))     return "0.0,9.0E-9,10,1\n";
    if (contains(cmd, ":DATA?"))           return rohdeFloatBlock();
    if (contains(cmd, "RESult:ACTual?"))   return "1.0\n";

    // LeCroy
    if (contains(cmd, "HORIZ_INTERVAL"))   return "C1:INSP \"HORIZ_INTERVAL : 1e-09\"\n";
    if (contains(cmd, "HORIZ_OFFSET"))     return "C1:INSP \"HORIZ_OFFSET : 0\"\n";
    if (contains(cmd, "SIMPLE"))           return lecroySimple();
    if (contains(cmd, "PAVA?"))            return "C1:PAVA PKPK,1.0E+00 V\n";

    // Keysight-style measurement ":MEASure:VPP? CHANnel1" etc.
    if (contains(cmd, "MEASure:") && contains(cmd, "?")) return "+2.5E+00\n";

    return "0\n"; // generic fallback
}

std::string trimTerminator(const std::string& s)
{
    std::string r = s;
    while (!r.empty() && (r.back() == '\n' || r.back() == '\r' || r.back() == ' ')) r.pop_back();
    return r;
}

} // namespace

// ---------------- test control surface ----------------
namespace visastub {
void reset() { g_commands.clear(); g_responses.clear(); g_readBuf.clear(); g_nextSession = 100; }
std::vector<std::string> commands() { return g_commands; }
bool sawCommand(const std::string& in_cmd)
{
    for (const auto& c : g_commands) if (c == in_cmd) return true;
    return false;
}
}

// ---------------- VISA API ----------------
extern "C" {

ViStatus viOpenDefaultRM(ViPSession vi) { *vi = 1; return VI_SUCCESS; }

ViStatus viOpen(ViSession, ViRsrc, ViAccessMode, ViUInt32, ViPSession vi)
{
    *vi = g_nextSession++;
    return VI_SUCCESS;
}

ViStatus viClose(ViSession) { return VI_SUCCESS; }

ViStatus viSetAttribute(ViSession, ViAttr, ViAttrState) { return VI_SUCCESS; }

ViStatus viWrite(ViSession, ViBuf buf, ViUInt32 count, ViUInt32* retCount)
{
    std::string cmd(reinterpret_cast<const char*>(buf), count);
    if (retCount) *retCount = count;
    const std::string logged = trimTerminator(cmd);
    if (!logged.empty()) g_commands.push_back(logged);
    // Queries carry '?'; LeCroy SCDP is a query-like command without one.
    if (contains(cmd, "?") || logged.rfind("SCDP", 0) == 0)
        g_responses.push_back(responseFor(logged));
    return VI_SUCCESS;
}

ViStatus viRead(ViSession, ViBuf buf, ViUInt32 count, ViUInt32* retCount)
{
    if (g_readBuf.empty()) {
        if (g_responses.empty()) { if (retCount) *retCount = 0; return VI_ERROR_TMO; }
        g_readBuf = g_responses.front();
        g_responses.pop_front();
    }
    const ViUInt32 n = (g_readBuf.size() < count) ? static_cast<ViUInt32>(g_readBuf.size()) : count;
    std::memcpy(buf, g_readBuf.data(), n);
    if (retCount) *retCount = n;
    const bool more = (n < g_readBuf.size());
    g_readBuf.erase(0, n);
    return more ? VI_SUCCESS_MAX_CNT : VI_SUCCESS;
}

} // extern "C"
