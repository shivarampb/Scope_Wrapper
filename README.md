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
    CScpiCommandBuilder.h     # numeric formatting + parameter validation
    ScopeDialect.h            # S_ScpiDialect: a vendor's SCPI command set as templates
    CVisaScopePlugin.h        # shared VISA transport + generic interface impl
    ScopeFamilies.h           # per-vendor dialect factories + family bases + builders
    ScopeManager.h            # CScopeManager singleton (QPluginLoader based)
  src/                        # matching .cpp implementations
plugins/                      # one loadable plugin per inventory model
  PluginKeysightDSOX2012A/  PluginKeysightDSO7104B/  PluginKeysightDSOS204A/
  PluginKeysightMSO6054A/   PluginTektronixMDO34/    PluginTektronixTDS2024C/
  PluginTektronixTDS1012B/  PluginRohdeSchwarzRTM3004/ PluginRohdeSchwarzRTO2064/
  PluginLeCroyWaveSurfer42XS/
models/
  scope_models.json           # Inventory-seeded model catalog
  DSOX2012A/scpi_map.json     # Reviewed SCPI command map for the model
datasheets/
  index.json                  # Programming-manual / datasheet index
ScopePlugins.pro              # Top-level qmake subdirs project
```

### How a plugin is built

The identical VISA transport (the code each ELoad_R2 plugin duplicated) is
factored once into `CVisaScopePlugin`, which implements the whole
`CIScopePlugin` interface generically, driven by an `S_ScpiDialect` command
table (the in-code form of `scpi_map.json`). Each **model plugin** is still its
own dynamically-loaded module (own `.pro`, `Q_PLUGIN_METADATA`, capabilities)
but reduces to identity + capabilities + a dialect:

```cpp
CKeysightDSOX2012APlugin::CKeysightDSOX2012APlugin() {
    m_info    = makeScopePluginInfo("Keysight DSOX2012A", ...);
    m_caps    = makeScopeCaps(2, 100e6, 2e9, 100000, 5e-9, 50.0, 1e-3, 5.0, false);
    m_dialect = keysightInfiniiVisionDialect();
}
```

Vendors whose waveform/measurement transfer differs from the Keysight-style
default override just those methods in a family base
(`CTektronixScopeBase`, `CRohdeSchwarzScopeBase`, `CLeCroyScopeBase`).

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

## Testing (no hardware required)

`tests/` contains a Qt Test suite that drives the real plugin code through an
in-process **SCPI oscilloscope simulator** (`tests/visastub/`, a VISA test
double). It validates command formatting, parameter validation, error strings,
VISA resource strings, version compatibility, and — per vendor dialect
(Keysight/Tektronix/R&S/LeCroy) — the exact SCPI commands generated and the
decoded waveform round-trip.

```sh
sudo apt-get install -y --no-install-recommends qtbase5-dev qtbase5-dev-tools
tests/run_tests.sh
# => Totals: 12 passed, 0 failed
```

The simulator swaps in for real VISA only under test; production builds link the
vendor VISA via each plugin's `.pro`.

## Supported models (all inventory scopes)

| Model | Vendor | BW | Ch | Plugin | Family base |
|-------|--------|----|----|--------|-------------|
| DSOX2012A | Keysight/Agilent | 100 MHz | 2 | PluginKeysightDSOX2012A | InfiniiVision |
| DSO7104B | Agilent | 1 GHz | 4 | PluginKeysightDSO7104B | InfiniiVision |
| DSOS204A | Keysight | 2 GHz | 4 | PluginKeysightDSOS204A | Infiniium |
| MSO6054A | Agilent | 500 MHz | 4 | PluginKeysightMSO6054A | InfiniiVision (MSO) |
| MDO34 | Tektronix | 1 GHz | 4 | PluginTektronixMDO34 | Tektronix |
| TDS2024C | Tektronix | 200 MHz | 4 | PluginTektronixTDS2024C | Tektronix |
| TDS1012B | Tektronix | 100 MHz | 2 | PluginTektronixTDS1012B | Tektronix |
| RTM3004 | Rohde & Schwarz | 1 GHz | 4 | PluginRohdeSchwarzRTM3004 | R&S |
| RTO2064 | Rohde & Schwarz | 6 GHz | 4 | PluginRohdeSchwarzRTO2064 | R&S |
| WaveSurfer 42XS | LeCroy | 400 MHz | 4 | PluginLeCroyWaveSurfer42XS | LeCroy |

## Capabilities

Per instrument, through `CIScopePlugin` / `CScopeManager`: connect/`*IDN?`/reset/
auto-setup; vertical (scale, offset, coupling, probe, bandwidth-limit, enable);
horizontal (timebase, position, **memory depth**, **sample rate**); trigger
(source, slope, level, mode); acquisition (mode, average, run/stop/single/force);
measurements; waveform capture; **screenshot**; **setup save/recall** (`*SAV`/`*RCL`);
**MSO digital channels** (enable + threshold, on MSO models); full status/error
queue. `CScopeManager::createInstanceFromIdn()` **auto-selects the plugin from an
`*IDN?` string** (normalized match, so `DSO-X 2012A` resolves `DSOX2012A`).

## Status

- **Complete for all 10 inventory models**: core library, plugins, the full
  feature set above, per-model `scpi_map.json` for every model, model catalog,
  datasheet index + `datasheet_fetch` tool, and CI.
- **22 automated tests green** — 16 unit/simulator (`tests/`) + 6 dynamic-load
  (`tests/dl/`, real `.so` via `QPluginLoader`), all host-free via the SCPI
  simulator. A test cross-checks generated commands against each `scpi_map.json`.
- SCPI dialects are coded to each vendor's programming manual and
  simulator-verified. The Keysight path is the most exercised; Tektronix/R&S/
  LeCroy waveform/measurement paths still want a **bench check** against real
  instruments.
- **Externally blocked (not in this repo):** validation on real hardware, and
  downloading actual datasheet PDFs (needs Digi-Key/Mouser API keys — the tool
  and index are ready to run once keys exist).
