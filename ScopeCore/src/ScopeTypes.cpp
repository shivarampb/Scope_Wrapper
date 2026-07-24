// ScopeTypes.cpp
//
// Out-of-line helpers for ScopeTypes structs. Currently just the semantic
// version comparison used by S_PluginInfo::isCompatible (mirrors the
// reference's version-gate used by the manager when loading a plugin).

#include "ScopeTypes.h"

namespace {

// Parse "MAJOR.MINOR.PATCH" into a comparable 3-tuple; missing fields => 0.
void parseVersion(const QString& in_kStr, int& out_iMajor, int& out_iMinor, int& out_iPatch)
{
    out_iMajor = out_iMinor = out_iPatch = 0;
    const QStringList parts = in_kStr.split('.', Qt::SkipEmptyParts);
    if (parts.size() > 0) out_iMajor = parts.at(0).toInt();
    if (parts.size() > 1) out_iMinor = parts.at(1).toInt();
    if (parts.size() > 2) out_iPatch = parts.at(2).toInt();
}

// Returns -1/0/+1 for a<b / a==b / a>b.
int compareVersion(const QString& in_kA, const QString& in_kB)
{
    int aMaj, aMin, aPat, bMaj, bMin, bPat;
    parseVersion(in_kA, aMaj, aMin, aPat);
    parseVersion(in_kB, bMaj, bMin, bPat);

    if (aMaj != bMaj) return aMaj < bMaj ? -1 : 1;
    if (aMin != bMin) return aMin < bMin ? -1 : 1;
    if (aPat != bPat) return aPat < bPat ? -1 : 1;
    return 0;
}

} // namespace

bool S_PluginInfo::isCompatible(const QString& in_kStrCoreVersion) const
{
    const QString strMin = QString::fromLocal8Bit(m_szMinCoreVersion);
    const QString strMax = QString::fromLocal8Bit(m_szMaxCoreVersion);

    if (!strMin.isEmpty() && compareVersion(in_kStrCoreVersion, strMin) < 0)
    {
        return false; // core older than plugin's minimum
    }
    if (!strMax.isEmpty() && compareVersion(in_kStrCoreVersion, strMax) > 0)
    {
        return false; // core newer than plugin's maximum
    }
    return true;
}
