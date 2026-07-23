// ScopeManager.cpp
//
// Implementation mirrors the ELoad_R2 `PowerSupplyManager.cpp`: platform plugin
// discovery (*.dll / *.so), QPluginLoader::instance() + qobject_cast, version
// gate, instance lifecycle, and thin delegating wrappers that resolve the bound
// plugin and forward the call.

#include "ScopeManager.h"
#include <QDir>
#include <QDebug>

QString getCoreVersion()
{
    return QStringLiteral("1.0.0");
}

CScopeManager& CScopeManager::instance()
{
    static CScopeManager objScopeInstance;
    return objScopeInstance;
}

CScopeManager::CScopeManager()
{
}

CScopeManager::~CScopeManager()
{
    m_instances.clear();

    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
    {
        if (it.value().m_QCPlugInLoader)
        {
            it.value().m_QCPlugInLoader->unload();
            delete it.value().m_QCPlugInLoader;
        }
    }
    m_plugins.clear();
}

void CScopeManager::loadPlugins(const QString& in_kStrPluginPath)
{
    QDir pluginsDir(in_kStrPluginPath);
    QStringList filters;

#ifdef Q_OS_WIN
    filters << "*.dll";
#elif defined(Q_OS_MAC)
    filters << "*.dylib";
#else
    filters << "*.so";
#endif
    pluginsDir.setNameFilters(filters);

    const auto entries = pluginsDir.entryList(QDir::Files);
    for (const QString& fileName : entries)
    {
        const QString StrFullPath = pluginsDir.absoluteFilePath(fileName);
        QPluginLoader* loader = new QPluginLoader(StrFullPath);

        QObject* pobjPlugin = loader->instance();
        if (!pobjPlugin)
        {
            emit pluginLoadFailed(fileName, loader->errorString());
            delete loader;
            continue;
        }

        CIScopePlugin* scopePlugin = qobject_cast<CIScopePlugin*>(pobjPlugin);
        if (!scopePlugin)
        {
            emit pluginLoadFailed(fileName, "Not a Scope plugin");
            loader->unload();
            delete loader;
            continue;
        }

        S_PluginInfo sPluginInfo = scopePlugin->getPluginInfo();
        if (!sPluginInfo.isCompatible(getCoreVersion()))
        {
            emit pluginLoadFailed(fileName, "Version incompatible");
            loader->unload();
            delete loader;
            continue;
        }

        S_PluginData sPluginData;
        sPluginData.m_QCPlugInLoader = loader;
        sPluginData.m_objScopePlugin = scopePlugin;
        sPluginData.m_sPluginInfo    = sPluginInfo;

        const QString name = QString::fromLocal8Bit(sPluginInfo.m_szName);
        m_plugins[name] = sPluginData;
        emit pluginLoaded(name);
        qDebug() << "Loaded scope plugin:" << name
                 << "version:" << QString::fromLocal8Bit(sPluginInfo.m_szVersion);
    }
}

QStringList CScopeManager::getAvailablePlugins() const
{
    return m_plugins.keys();
}

S_PluginInfo CScopeManager::getPluginInfo(const QString& in_kStrPluginName) const
{
    if (m_plugins.contains(in_kStrPluginName))
    {
        return m_plugins[in_kStrPluginName].m_sPluginInfo;
    }
    return S_PluginInfo();
}

ScopeError CScopeManager::createInstance(U32BIT in_u32ScopeNumber, const QString& in_kStrPluginName)
{
    if (in_u32ScopeNumber < 1)
    {
        return ScopeError(ScopeErrorCode::INVALID_SCOPE_NUMBER, "Scope number must be >= 1");
    }
    if (m_instances.contains(in_u32ScopeNumber))
    {
        return ScopeError(ScopeErrorCode::ALREADY_CONNECTED,
                          QString("Scope number %1 already in use").arg(in_u32ScopeNumber));
    }
    if (!m_plugins.contains(in_kStrPluginName))
    {
        return ScopeError(ScopeErrorCode::PLUGIN_NOT_FOUND,
                          QString("Plugin '%1' not found").arg(in_kStrPluginName));
    }

    m_instances[in_u32ScopeNumber] = in_kStrPluginName;
    emit instanceCreated(in_u32ScopeNumber, in_kStrPluginName);
    return ScopeError(ScopeErrorCode::SUCCESS);
}

ScopeError CScopeManager::destroyInstance(U32BIT in_u32ScopeNumber)
{
    if (!m_instances.contains(in_u32ScopeNumber))
    {
        return ScopeError(ScopeErrorCode::INVALID_SCOPE_NUMBER,
                          QString("Scope number %1 not found").arg(in_u32ScopeNumber));
    }

    CIScopePlugin* plugin = getPlugin(in_u32ScopeNumber);
    if (plugin && plugin->isConnected(in_u32ScopeNumber))
    {
        plugin->disconnect(in_u32ScopeNumber);
    }

    m_instances.remove(in_u32ScopeNumber);
    emit instanceDestroyed(in_u32ScopeNumber);
    return ScopeError(ScopeErrorCode::SUCCESS);
}

bool CScopeManager::instanceExists(U32BIT in_u32ScopeNumber) const
{
    return m_instances.contains(in_u32ScopeNumber);
}

QString CScopeManager::getInstancePlugin(U32BIT in_u32ScopeNumber) const
{
    return m_instances.value(in_u32ScopeNumber, "");
}

CIScopePlugin* CScopeManager::getPlugin(U32BIT in_u32ScopeNumber)
{
    if (!m_instances.contains(in_u32ScopeNumber))
    {
        return nullptr;
    }
    const QString pluginName = m_instances[in_u32ScopeNumber];
    if (!m_plugins.contains(pluginName))
    {
        return nullptr;
    }
    return m_plugins[pluginName].m_objScopePlugin;
}

const CIScopePlugin* CScopeManager::getPlugin(U32BIT in_u32ScopeNumber) const
{
    if (!m_instances.contains(in_u32ScopeNumber))
    {
        return nullptr;
    }
    const QString pluginName = m_instances[in_u32ScopeNumber];
    if (!m_plugins.contains(pluginName))
    {
        return nullptr;
    }
    return m_plugins[pluginName].m_objScopePlugin;
}

// ---- Delegating wrappers ----
// Pattern is identical for every call: resolve plugin, guard, forward.

#define SCOPE_RESOLVE_OR_FAIL(pluginVar, scopeNum)                       \
    CIScopePlugin* pluginVar = getPlugin(scopeNum);                      \
    if (!pluginVar)                                                      \
    {                                                                    \
        return ScopeError(ScopeErrorCode::INVALID_SCOPE_NUMBER,         \
                          QString("Scope number %1 not found").arg(scopeNum)); \
    }

ScopeError CScopeManager::connect(U32BIT in_u32ScopeNumber, const S_ConnectionConfig& in_sConfig)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->connect(in_u32ScopeNumber, in_sConfig);
}

ScopeError CScopeManager::disconnect(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->disconnect(in_u32ScopeNumber);
}

bool CScopeManager::isConnected(U32BIT in_u32ScopeNumber) const
{
    const CIScopePlugin* plugin = getPlugin(in_u32ScopeNumber);
    return plugin ? plugin->isConnected(in_u32ScopeNumber) : false;
}

ScopeError CScopeManager::reset(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->reset(in_u32ScopeNumber);
}

ScopeError CScopeManager::autoSetup(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->autoSetup(in_u32ScopeNumber);
}

ScopeError CScopeManager::setVerticalScale(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, FDOUBLE in_dVoltsPerDiv)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->setVerticalScale(in_u32ScopeNumber, in_u32Channel, in_dVoltsPerDiv);
}

ScopeError CScopeManager::setTimebaseScale(U32BIT in_u32ScopeNumber, FDOUBLE in_dSecondsPerDiv)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->setTimebaseScale(in_u32ScopeNumber, in_dSecondsPerDiv);
}

ScopeError CScopeManager::setTrigger(U32BIT in_u32ScopeNumber, const S_TriggerConfig& in_sTrigger)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->setTrigger(in_u32ScopeNumber, in_sTrigger);
}

ScopeError CScopeManager::single(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->single(in_u32ScopeNumber);
}

ScopeError CScopeManager::run(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->run(in_u32ScopeNumber);
}

ScopeError CScopeManager::stop(U32BIT in_u32ScopeNumber)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->stop(in_u32ScopeNumber);
}

ScopeError CScopeManager::measure(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel,
                                  Enum_MeasurementType in_enumType, FDOUBLE& out_dValue)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->measure(in_u32ScopeNumber, in_u32Channel, in_enumType, out_dValue);
}

ScopeError CScopeManager::captureWaveform(U32BIT in_u32ScopeNumber, U32BIT in_u32Channel, S_Waveform& out_sWaveform)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->captureWaveform(in_u32ScopeNumber, in_u32Channel, out_sWaveform);
}

ScopeError CScopeManager::readErrorStatus(U32BIT in_u32ScopeNumber, S_DeviceErrorStatus& out_sStatus)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    ScopeError err = plugin->readErrorStatus(in_u32ScopeNumber, out_sStatus);
    if (err.isSuccess() && out_sStatus.hasError())
    {
        emit errorOccurred(in_u32ScopeNumber, out_sStatus);
    }
    return err;
}

ScopeError CScopeManager::queryErrorQueue(U32BIT in_u32ScopeNumber, QString& out_qstrErrorMessage)
{
    SCOPE_RESOLVE_OR_FAIL(plugin, in_u32ScopeNumber);
    return plugin->queryErrorQueue(in_u32ScopeNumber, out_qstrErrorMessage);
}

#undef SCOPE_RESOLVE_OR_FAIL
