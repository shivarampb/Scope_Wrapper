// ScopeManager.h
//
// Singleton facade that loads scope plugins dynamically and delegates all
// operations to the plugin bound to a given scope instance. Direct analogue of
// the ELoad_R2 `CPowerSupplyManager`: same loadPlugins()/QPluginLoader design,
// version-compat gate, instance map (scope number -> plugin name), delegating
// wrappers, and Qt signals for lifecycle/error notification.

#ifndef SCOPEMANAGER_H
#define SCOPEMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QPluginLoader>
#include "dp_types.h"
#include "IScopePlugin.h"
#include "ScopeError.h"
#include "ScopeTypes.h"

// Core version this build presents to plugins during compatibility checks.
QString getCoreVersion();

class CScopeManager : public QObject {
    Q_OBJECT

public:
    CScopeManager();
    ~CScopeManager();

    static CScopeManager& instance();

    // ---- Plugin management ----
    void        loadPlugins(const QString& in_kStrPluginPath);
    QStringList getAvailablePlugins() const;
    S_PluginInfo getPluginInfo(const QString& in_kStrPluginName) const;

    // ---- Instance management (scope number -> plugin) ----
    ScopeError createInstance(U32BIT in_u32ScopeNumber, const QString& in_kStrPluginName);
    ScopeError destroyInstance(U32BIT in_u32ScopeNumber);
    bool       instanceExists(U32BIT in_u32ScopeNumber) const;
    QString    getInstancePlugin(U32BIT in_u32ScopeNumber) const;

    // Auto-selection: match a loaded plugin to an *IDN? / model string, and
    // create an instance for it in one step.
    QString    matchPluginForModel(const QString& in_kIdnOrModel) const;
    ScopeError createInstanceFromIdn(U32BIT in_u32ScopeNumber, const QString& in_kIdn);

    // ---- Delegating operations (subset; extend 1:1 with the interface) ----
    ScopeError connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig);
    ScopeError disconnect(U32BIT in_u32ScopeNumber);
    bool       isConnected(U32BIT in_u32ScopeNumber) const;
    ScopeError reset(U32BIT in_u32ScopeNumber);
    ScopeError autoSetup(U32BIT in_u32ScopeNumber);

    ScopeError setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv);
    ScopeError setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv);
    ScopeError setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger);
    ScopeError single(U32BIT in_u32ScopeNumber);
    ScopeError run(U32BIT in_u32ScopeNumber);
    ScopeError stop(U32BIT in_u32ScopeNumber);
    ScopeError measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                       Enum_MeasurementType in_enumType, FDOUBLE& out_dValue);
    ScopeError captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform);
    ScopeError setMemoryDepth(U32BIT in_u32ScopeNumber, U64BIT in_u64Points);
    ScopeError getScreenshot(U32BIT in_u32ScopeNumber, QByteArray& out_baImage);
    ScopeError saveSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);
    ScopeError recallSetup(U32BIT in_u32ScopeNumber, U32BIT in_u32Location);

    ScopeError readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus);
    ScopeError queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage);

signals:
    void pluginLoaded(const QString& pluginName);
    void pluginLoadFailed(const QString& fileName, const QString& error);
    void instanceCreated(U32BIT in_u32ScopeNumber, const QString& pluginName);
    void instanceDestroyed(U32BIT in_u32ScopeNumber);
    void errorOccurred(U32BIT in_u32ScopeNumber, const S_DeviceErrorStatus& status);

private:
    CScopeManager(const CScopeManager&) = delete;
    CScopeManager& operator=(const CScopeManager&) = delete;

    CIScopePlugin* getPlugin(U32BIT in_u32ScopeNumber);
    const CIScopePlugin* getPlugin(U32BIT in_u32ScopeNumber) const;

    struct S_PluginData {
        QPluginLoader* m_QCPlugInLoader;
        CIScopePlugin* m_objScopePlugin;
        S_PluginInfo   m_sPluginInfo;

        S_PluginData() : m_QCPlugInLoader(nullptr), m_objScopePlugin(nullptr) {}
    };

    QMap<QString, S_PluginData> m_plugins;    // pluginName -> loaded plugin
    QMap<U32BIT, QString>       m_instances;  // scopeNumber -> pluginName
};

#endif // SCOPEMANAGER_H
