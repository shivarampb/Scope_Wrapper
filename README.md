# Scope Wrapper — Oscilloscope Plugin System

A Qt C++ plugin system for controlling digital oscilloscopes over VISA, built as
a **structural replica of the `ELoad_R2`** power-supply/electronic-load plugin
architecture. A shared `ScopeCore` library defines the interface, a singleton
manager, and the error/type/transport model; per-model plugins (dynamically
loaded `.so`/`.dll`) each encapsulate one instrument family's SCPI dialect,
capabilities, and error mapping.

See [`SCOPE_PLUGIN_DEVELOPMENT_PLAN.md`](SCOPE_PLUGIN_DEVELOPMENT_PLAN.md) for the
full design and phasing.

## Layout

```
ScopeCore/                    # Shared core library (analogue of PowerSupplyCore)
  include/
    dp_types.h                # U32BIT / FDOUBLE / S8BIT primitive aliases
    ScopeTypes.h              # S_PluginInfo, S_ScopeCapabilities, S_Waveform, S_ConnectionConfig, enums
    ScopeError.h              # ScopeErrorCode, DeviceStatusFlag, S_DeviceErrorStatus, ScopeError
    VisaHelper.h              # S_ConnectionConfig::toVisaResourceString() (reused from reference)
    IScopePlugin.h            # CIScopePlugin interface + Q_DECLARE_INTERFACE
    CScpiCommandBuilder.h     # command templating + parameter validation
    ScopeManager.h            # CScopeManager singleton (QPluginLoader based)
  src/                        # matching .cpp implementations
plugins/
  PluginKeysightDSOX2012A/    # First reference plugin (100 MHz, 2 ch)
models/
  scope_models.json           # Inventory-seeded model catalog
  DSOX2012A/scpi_map.json     # Reviewed SCPI command map for the model
datasheets/
  index.json                  # Programming-manual / datasheet index
ScopePlugins.pro              # Top-level qmake subdirs project
```

## Mapping to ELoad_R2

| ELoad_R2 (reference)        | Scope Wrapper                         |
|-----------------------------|---------------------------------------|
| `CIPowerSupplyPlugin`       | `CIScopePlugin`                       |
| `CPowerSupplyManager`       | `CScopeManager`                       |
| `PSErrorCode` / `PowerSupplyError` | `ScopeErrorCode` / `ScopeError` |
| `Enum_OperatingMode` (CV/CC)| `Enum_OperatingState` (Run/Stop/Single) |
| `S_PowerSupplyCapabilities` | `S_ScopeCapabilities`                 |
| `setOVP` / `setUVP` / …     | `setVerticalScale` / `setTimebaseScale` / `setTrigger` / `captureWaveform` / … |
| `VisaHelper`                | reused nearly verbatim                |
| `PluginTDKLambdaZUP36_6`    | `PluginKeysightDSOX2012A`             |

Conventions carried over: `C`-prefixed classes, `S_` structs, `Enum_` enums,
`U32BIT`/`FDOUBLE`/`S8BIT`, `in_`/`out_` parameters, VISA as the sole transport,
value-type error objects (no exceptions), Qt signals for async status.

## Build

Requires Qt 5/6 and a VISA implementation (Keysight IO Libraries, NI-VISA, or
`librevisa`/`linux-gpib` on Linux).

```sh
qmake ScopePlugins.pro
make
# => lib/libScopeCore.*   and   bin/plugins/libPluginKeysightDSOX2012A.*
```

At runtime, `CScopeManager::instance().loadPlugins("bin/plugins")` discovers and
registers every built plugin.

## Status

- **Phase 1 (core + first plugin): implemented in this branch.**
- Phases 2–4 (datasheet tooling, more fleet plugins, measurement suite, tests)
  are described in the development plan.
