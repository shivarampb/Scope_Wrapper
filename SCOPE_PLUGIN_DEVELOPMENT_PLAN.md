# Scope Plugin Development Plan

## Context

The `Scope_Wrapper` repository is currently greenfield — it contains only
`Equipment_Inventory_List.xlsx`, which enumerates the target oscilloscope fleet
(Keysight/Agilent DSO7104B, DSOS204A, DSOX2012A, MSO6054A; Tektronix MDO34,
TDS1012B, TDS2024C and other DPO/MSO units; Rohde & Schwarz RTM3004, RTO2064;
LeCroy WaveSurfer 42XS). The goal is a **scope control plugin system that is a
structural replica of `shivarampb/ELoad_R2`**, adapted from power
supplies / electronic loads to digital oscilloscopes.

`ELoad_R2`'s `PluginSample.txt` (cloned to `/workspace/eload_r2`) is the
authoritative reference. It defines a **Qt C++ plugin architecture**: a shared
core library (interface + manager + error/type modules + VISA helper) plus
per-instrument dynamically-loaded plugins (`.so`/`.dll`) loaded via
`QPluginLoader`. This plan mirrors that architecture 1:1 for oscilloscopes,
preserving its module layout, class/struct naming conventions, the singleton
manager + dynamic-plugin pattern, the VISA transport layer, and the
capabilities/error/status model — swapping the power-supply domain (voltage,
current, OVP/UVP, CV/CC) for the scope domain (vertical, horizontal, trigger,
acquisition, waveform, measurement).

**Why:** to give the lab a uniform, extensible, host-agnostic way to drive any
scope in the inventory through one common interface, matching the established
`ELoad_R2` design so both instrument families share tooling, conventions, and
integration surface.

---

## 1. Introduction and Objectives

Build **`ScopeCore`** (a shared Qt C++ library) plus a family of per-model
**scope plugins**, replicating the exact structure, patterns, and naming of
`ELoad_R2`.

Objectives:
- **Exact structural replica** of `ELoad_R2`: same module decomposition,
  singleton manager, `QPluginLoader`-based dynamic plugins, VISA transport,
  capabilities/error/status model, and coding conventions (`C`-classes, `S_`
  structs, `Enum_` enums, `U32BIT`/`FDOUBLE`/`S8BIT`, `in_`/`out_` params).
- **Domain re-target** from PSU/eload → oscilloscope: vertical, horizontal,
  trigger, acquisition, measurement, waveform transfer.
- **Datasheet-driven SCPI generation**: a documented pipeline from vendor
  programming manuals (with Digi-Key/Mouser as a datasheet-retrieval fallback)
  to per-model SCPI command maps.
- **Coverage of the inventory fleet** as concrete first plugins.

---

## 2. ELoad_R2 Architecture Analysis Summary

From `/workspace/eload_r2/PluginSample.txt`, the reference is built from these
components (all to be mirrored):

| Reference file | Role |
|---|---|
| `IPowerSupplyPlugin.h` | Abstract interface `CIPowerSupplyPlugin`, `Q_DECLARE_INTERFACE`, IID `com.automation.PowerSupplyPlugin/1.0`. Pure-virtual API: `getPluginInfo`, `getCapabilities`, `connect/disconnect/isConnected/reset`, `setAddress`, `checkOperationComplete`, protection & measurement methods. |
| `PowerSupplyManager.h/.cpp` | Singleton `CPowerSupplyManager : QObject`. `loadPlugins()` scans a dir for `*.so`/`*.dll`, `QPluginLoader::instance()` + `qobject_cast`, version-compat check (`S_PluginInfo::isCompatible`), stores `S_PluginData{loader,plugin,info}` in `QMap`. Maps instance id → plugin name (`m_instances`). Delegates every call to the resolved plugin; emits `pluginLoaded`, `instanceCreated`, `errorOccurred`, etc. |
| `PowerSupplyError.h/.cpp` | `enum class PSErrorCode` (banded: connection 1000s, command 2000s, param 3000s, device 4000s, plugin 5000s), `DeviceStatusFlag` `Q_DECLARE_FLAGS`, `Enum_OperatingMode` (CV/CC/UR), `S_DeviceErrorStatus` (status flags + ESR/QUES/OPER registers), `PowerSupplyError` value type (code+description, `toString`, `isSuccess`, code→string helpers). |
| `PowerSupplyTypes.h` (referenced) | `S_PluginInfo` (fixed `char[]` fields sized by `PLUGIN_INFO_*_SIZE`, supported-protocol list, `isCompatible`), `S_PowerSupplyCapabilities` + `S_ChannelCapabilities` (per-channel limits/features, `QList` of channels), `S_ConnectionConfig`, `Enum_CommunicationProtocol` (RS232/RS485/GPIB/USB/TCPIP/ETHERNET/LXI/VXI11). |
| `VisaHelper.h` | `S_ConnectionConfig::toVisaResourceString()` — builds VISA resource strings per protocol (`ASRL…::INSTR`, `GPIB…::INSTR`, `USB0::…::INSTR`, `TCPIP0::…::INSTR/SOCKET`, VXI-11). |
| `PluginTDKLambdaZUP36_6.pro` | qmake `TEMPLATE=lib`, `CONFIG+=plugin c++11`, `DESTDIR=../plugins`, `INCLUDEPATH+=../include`, `LIBS+=-L../lib -lPowerSupplyCore` + VISA libs (win32 IVI VISA / unix `-lvisa`). |
| `TDKLambdaZUP36_6Plugin.h/.cpp` | Concrete plugin: `QObject`+interface, `Q_PLUGIN_METADATA(IID …)`, `Q_INTERFACES`. Per-model `getPluginInfo`/`getCapabilities` (hardcoded specs), `connect` (`viOpenDefaultRM`→`viOpen`→`viSetAttribute`→`*IDN?`), `sendCommand` (`viWrite`/`viRead`, query vs command handling), `buildCommand` (address/daisy-chain prefixing), `parse<Vendor>Error` (maps device error codes → `PSErrorCode`), `visaErrorToPS`, `validateResponse`, and the SCPI setters. Per-instance `S_DeviceInstance{ViSession vi, defaultRM, config, connected}` kept in a `QMap`. |

**Design patterns to preserve:** singleton facade manager; interface-segregated
dynamically-loaded plugins; one plugin = one instrument family owning its SCPI
dialect + error map + capabilities; VISA as the sole transport; value-type
error objects returned everywhere (no exceptions); Qt signals for async status;
POD `S_` structs with fixed-size buffers for ABI stability across the plugin
boundary.

---

## 3. Scope Plugin Core Requirements

Analogous scope functionality (interface surface of `CIScopePlugin`):

* **Plugin identity & capabilities** — `getPluginInfo()`, `getCapabilities()`
  (channels, analog bandwidth, max sample rate, memory depth, min/max
  V/div & s/div, has-FFT, has-digital/MSO, supported trigger types).
* **Connection management** — `connect`, `disconnect`, `isConnected`, `reset`,
  `setAddress`, `checkOperationComplete` (`*OPC?`), `autoSetup` (`:AUToset`).
* **Vertical** — `setVerticalScale` (V/div), `setVerticalOffset`/`setPosition`,
  `setCoupling` (AC/DC/GND), `setProbeAttenuation`, `setBandwidthLimit`,
  `setChannelEnable`.
* **Horizontal / timebase** — `setTimebaseScale` (s/div),
  `setHorizontalPosition`/delay, `setSampleRate`, `setMemoryDepth`.
* **Trigger** — `setTriggerSource`, `setTriggerType` (edge/pulse/…),
  `setTriggerEdgeSlope` (rising/falling/either), `setTriggerLevel`,
  `setTriggerMode` (auto/normal/single).
* **Acquisition control** — `setAcquireMode` (sample/peak/average/hi-res),
  `setAverageCount`, `run`, `stop`, `single`, `forceTrigger`.
* **Measurements** — `measureVpp`, `measureVamp`, `measureVrms`,
  `measureFrequency`, `measurePeriod`, `measureRiseTime`, `measureFallTime`,
  `measureDutyCycle` (generic `measure(source, Enum_MeasType, out&)`).
* **Waveform transfer** — `captureWaveform(source, S_Waveform&)` (preamble +
  raw block → scaled samples), `getScreenshot` (optional).
* **Status/error** — `readErrorStatus`, `clearErrorStatus`, `queryErrorQueue`
  (`:SYSTem:ERRor?`), `readStandardEventStatus` (`*ESR?`), `readStatusByte`
  (`*STB?`), `readQuestionable/OperationStatus`.

Data models (Section 7 details each):
`S_PluginInfo`, `S_ScopeCapabilities`, `S_ChannelCapabilities`,
`S_TriggerCapabilities`, `S_ConnectionConfig`, `S_Waveform`,
`S_WaveformPreamble`, `S_DeviceErrorStatus`, and the enums
`Enum_AcquisitionMode`, `Enum_TriggerType`, `Enum_TriggerSlope`,
`Enum_Coupling`, `Enum_MeasurementType`, `Enum_CommunicationProtocol`.

---

## 4. Instrument Data Acquisition Strategy

Mirrors `ELoad_R2`, where each plugin hardcodes its `S_PluginInfo` /
capabilities and identity is confirmed at connect-time via `*IDN?`.

* **Source(s):**
  - **Primary (compile-time):** per-model capabilities baked into each
    plugin's `getCapabilities()`/`getPluginInfo()` — exactly as
    `TDKLambdaZUP36_6Plugin` does. This is the replica's canonical approach.
  - **Runtime discovery:** `*IDN?` on connect returns `<vendor>,<model>,<serial>,<fw>`;
    `CScopeManager` uses vendor/model to auto-select the matching plugin.
  - **Seed inventory:** parse `Equipment_Inventory_List.xlsx` once into a
    static `models/scope_models.json` catalog (model → vendor → bandwidth →
    channels → plugin name) to drive which plugins to build first and to power
    an optional auto-mapping table. (Fields available today: `MODEL NUMBER`,
    `MAKE`, `RANGE`/bandwidth, `SERIAL_NUMBER`.)
* **Data Structure:** `S_PluginInfo` (fixed `char[]` buffers +
  `QStringList m_StrlstSupportedProtocols` + `isCompatible()`), replicated
  verbatim; `S_ScopeCapabilities`/`S_ChannelCapabilities` replace the PSU
  capability structs. Optional external catalog as JSON validated against a
  documented schema.

---

## 5. Datasheet Management and Integration

> Note (flagged with user): Digi-Key/Mouser primarily index *components*, not
> bench scopes; authoritative SCPI syntax lives in **vendor programming
> manuals**. Plan therefore treats vendor manuals as primary and DK/Mouser as a
> datasheet-retrieval fallback. This mirrors `ELoad_R2`, whose repo carries the
> **Kikusui/TDK PLZ & interface manuals** as the SCPI source of truth
> (`PLZ_5WH2_M_copy.pdf`, `plz-5wh2_interface_manual_copy.pdf`).

* **Sources:**
  - Primary: Keysight, Tektronix, Rohde & Schwarz, LeCroy programming/SCPI
    reference manuals.
  - Fallback/aux: Digi-Key & Mouser product pages + datasheet PDFs (for
    part-level specs and locating the vendor doc link).
* **Acquisition Method:**
  - **Manual-first, versioned:** store each model's manual under `datasheets/<vendor>/<model>/`
    (as `ELoad_R2` does with the PLZ PDFs), tracked with a
    `datasheets/index.json` (model, doc title, version/date, source URL, sha256).
  - **Semi-automated retrieval helper** (`tools/datasheet_fetch`): a small
    Qt/CLI utility that, given a model or part number, queries the **Digi-Key
    Product Information API** and **Mouser Search API** (both offer official
    keyed REST APIs — no scraping) to resolve the datasheet URL and download the
    latest PDF into the tree, updating `index.json`. Respect each API's ToS and
    rate limits; keys via env/config, never committed.
* **Data Extraction:** extract SCPI syntax into a per-model **command map**
  (`models/<model>/scpi_map.json`): logical function → command template →
  params (type/unit/range/enum) → response parser hint → error semantics.
  Extraction is **human-reviewed**, PDF text pulled with `pdftotext`/`poppler`;
  each entry cites the manual section (same discipline as the ZUP plugin's
  `// ZUP Manual Section 5.5.5` comments on its error codes). The reviewed
  `scpi_map.json` is what a plugin's methods are coded against.

---

## 6. SCPI Command Generation Engine

* **Command Derivation Logic:** each plugin owns a `buildCommand()`
  (replicating `ZUP::buildCommand`) that composes the wire string from the
  per-model `scpi_map` template + runtime parameters, applying any model
  quirks (channel prefixing, header on/off, terminators). Commands are built,
  not free-typed — a `CScpiCommandBuilder` helper in `ScopeCore` fills templates
  like `:CHANnel%1:SCALe %2` and formats numerics with explicit precision/units
  (mirroring `QString(":CUR%1;").arg(v,0,'f',3)`).
* **Parameter Handling:** validate every value against
  `S_ScopeCapabilities`/`S_ChannelCapabilities` *before* transmit (range,
  enum membership, channel existence) — returning `ScopeErrorCode::PARAMETER_OUT_OF_RANGE`
  / `INVALID_CHANNEL` exactly as the reference does. Units normalized to SI in
  the API; formatting to the model's expected notation happens in `buildCommand`.
* **Error Handling and Validation:**
  - VISA-layer: `visaErrorToPS`-analogue `visaErrorToScope(ViStatus,…)` maps
    `VI_ERROR_*` → `ScopeErrorCode`.
  - Device-layer: `parse<Vendor>Error()` per plugin, plus a shared
    `:SYSTem:ERRor?` drain that maps `<code>,"<msg>"` into
    `S_DeviceErrorStatus`; `checkOperationComplete()` uses `*OPC?`.
  - Response validation: `validateResponse()` per plugin (reject error tokens,
    verify shape) as in the reference; queries flagged by `?` in `sendCommand`.

---

## 7. Architectural Adaptation from ELoad_R2

Replica lives under a new `ScopeCore` library + `plugins/`. Directory shape
mirrors the reference (`../include`, `../lib`, `../plugins`, per-model `.pro`).

* **`CIScopePlugin` (interface)** ⟵ `CIPowerSupplyPlugin`. New IID
  `com.automation.ScopePlugin/1.0`, `Q_DECLARE_INTERFACE`. Method set from
  Section 3. Instance handle param renamed `in_u32ScopeNumber` (was `PsNumber`).
* **`CScopeManager` (singleton)** ⟵ `CPowerSupplyManager`. Identical
  `loadPlugins`/`QPluginLoader`/version-check/instance-map/delegate/signals
  design; `createInstance`, `connect`, delegating wrappers, and signals
  (`pluginLoaded`, `instanceCreated`, `errorOccurred`, …) carried over verbatim
  in structure.
* **`ScopeError` module** ⟵ `PowerSupplyError`. `enum class ScopeErrorCode`
  reuses the banding (connection 1000s, command 2000s, param 3000s, device
  4000s, plugin 5000s) and adds scope-specific codes
  (`ACQUISITION_TIMEOUT`, `NO_TRIGGER`, `WAVEFORM_TRANSFER_FAILED`,
  `SET_TIMEBASE_FAILED`, `SET_TRIGGER_FAILED`, `INVALID_SOURCE`).
  `DeviceStatusFlag` `Q_DECLARE_FLAGS` retained (scope flags: `Triggered`,
  `Armed`, `AutoTrigger`, `Overload`, `ClippingPos/Neg`, `CommandError`, …).
  `Enum_OperatingMode` → `Enum_AcquisitionMode`. `S_DeviceErrorStatus`
  (ESR/QUES/OPER registers) and `ScopeError` value type carried over.
* **`ScopeTypes` module** ⟵ `PowerSupplyTypes`. `S_PluginInfo` reused as-is;
  `S_ScopeCapabilities`/`S_ChannelCapabilities` replace PSU caps
  (bandwidth, sampleRate, memoryDepth, V/div & s/div ranges, coupling set,
  trigger types, isMSO); **new** `S_Waveform`/`S_WaveformPreamble` (points,
  x-increment/origin, y-increment/origin/reference, format) and
  `S_TriggerConfig`. `S_ConnectionConfig` + `Enum_CommunicationProtocol`
  reused unchanged.
* **`VisaHelper`** ⟵ reused nearly **verbatim** — `toVisaResourceString()` is
  transport-only and instrument-agnostic; scopes are predominantly
  USB-TMC/LXI(TCPIP)/GPIB, all already handled.
* **Per-model plugins** ⟵ `TDKLambdaZUP36_6Plugin`. One `lib`/`plugin` `.pro`
  each, `Q_PLUGIN_METADATA`, `S_DeviceInstance{ViSession…}` map, `sendCommand`,
  `buildCommand`, `parse<Vendor>Error`, `getCapabilities`. First set, driven by
  the inventory: `PluginKeysightDSOX2012A`, `PluginKeysightMSO6054A`,
  `PluginTektronixMDO34`, `PluginTektronixTDS2024C`,
  `PluginRohdeSchwarzRTM3004`, `PluginLeCroyWaveSurfer42XS`.
* **Configuration Management:** as in the reference, per-model specs live in the
  plugin's `getCapabilities()` (canonical). External, editable data
  (SCPI maps, datasheet index, model catalog) lives under `models/` and
  `datasheets/` as versioned JSON — the reproducible analogue of the reference
  keeping its instrument manuals in-repo. Runtime `S_ConnectionConfig` supplied
  by the host per instance.

---

## 8. Implementation Phases

* **Phase 1: Foundation & Instrument Definition**
  * 1.1 Scaffold repo: `ScopeCore/` (`../include`, `../lib`), `plugins/`,
    `models/`, `datasheets/`, top-level qmake subdirs `.pro` (or CMake mirror).
  * 1.2 Port `dp_types` (`U32BIT`/`FDOUBLE`/`S8BIT`), `ScopeTypes`
    (`S_PluginInfo`, `S_ScopeCapabilities`, `S_ChannelCapabilities`,
    `S_Waveform*`, `S_ConnectionConfig`, enums), and `VisaHelper`.
  * 1.3 Implement `ScopeError` module (enum + flags + `S_DeviceErrorStatus` +
    value type + to-string helpers).
  * 1.4 Define `CIScopePlugin` interface (IID + `Q_DECLARE_INTERFACE`).
  * 1.5 Implement `CScopeManager` (load/version-check/instance/delegate/signals).
  * 1.6 Build `models/scope_models.json` from `Equipment_Inventory_List.xlsx`.

* **Phase 2: Datasheet & SCPI Engine Integration**
  * 2.1 Stand up `datasheets/` tree + `index.json`; add manuals for the first
    target models.
  * 2.2 Build `tools/datasheet_fetch` (Digi-Key + Mouser API clients, keyed).
  * 2.3 Author reviewed `models/<model>/scpi_map.json` for the first models.
  * 2.4 Implement `CScpiCommandBuilder` + parameter validation against caps.

* **Phase 3: Communication & Core Control**
  * 3.1 First reference plugin `PluginKeysightDSOX2012A` end-to-end
    (`connect`/`*IDN?`, `sendCommand`, `visaErrorToScope`, `parse…Error`).
  * 3.2 Vertical/horizontal/trigger/acquisition setters + `run/stop/single`.
  * 3.3 `captureWaveform` (preamble query + block read + scaling).
  * 3.4 Second vendor plugin (`PluginTektronixMDO34`) to prove the abstraction
    across SCPI dialects.

* **Phase 4: Advanced Features & Testing**
  * 4.1 Remaining fleet plugins (R&S RTM3004, LeCroy WaveSurfer 42XS, more
    Keysight/Tek).
  * 4.2 Measurements suite + status/error queue integration + `*OPC?` sync.
  * 4.3 Optional headless demo/host harness driving `CScopeManager`
    (mirrors `ELoad_R2` `QT -= gui` libraries; GUI only if the host requires).
  * 4.4 Full test pass (Section 9), datasheet cross-validation, docs.

---

## 9. Testing and Validation Strategy

* **Unit Testing (Qt Test):** `CScopeManager` load/version-compat/instance
  lifecycle with a mock `CIScopePlugin`; `ScopeError` mappings;
  `toVisaResourceString()` for every `Enum_CommunicationProtocol`;
  `CScpiCommandBuilder` template/format/precision; parameter validation
  (range/enum/channel) → correct `ScopeErrorCode`; `parse<Vendor>Error` and
  waveform preamble scaling math.
* **Integration Testing:**
  - **Simulated:** a **SCPI simulator** (Keysight/Tek offer instrument
    simulators; or a scripted TCP `SOCKET` responder implementing
    `*IDN?`/`:SYSTem:ERRor?`/preamble/waveform) validates the transport +
    plugin round-trip without hardware.
  - **Hardware:** smoke tests against real inventory units over LXI/USB-TMC:
    connect → autoscale → configure → single trigger → `captureWaveform` →
    measure, asserting against a known signal source.
* **Validation against Datasheets:** every SCPI string a plugin emits is
  cross-checked against its `scpi_map.json`, which cites the vendor manual
  section (the reference's `// … Manual Section x.y` discipline). A test
  asserts generated commands match the map; a review checklist ties each
  plugin method to a manual page.

---

## 10. Future Considerations (Optional)

* Auto-plugin-selection from `*IDN?` via the model catalog.
* Async waveform streaming / long-memory chunked transfer; binary block
  (`#` header) fast path.
* Screenshot/hardcopy retrieval; digital (MSO) channels; FFT/math.
* Cross-instrument sequencing shared with `ELoad_R2` (common core → unified
  test-automation host).
* CMake alongside qmake; CI building all plugins headless against the simulator.
* Session save/recall (`*SAV`/`*RCL`) and setup import/export.

---

## Critical Files / Artifacts to Create

- `ScopeCore/include/`: `IScopePlugin.h`, `ScopeManager.h`, `ScopeError.h`,
  `ScopeTypes.h`, `VisaHelper.h`, `dp_types.h`, `CScpiCommandBuilder.h`
- `ScopeCore/src/`: `ScopeManager.cpp`, `ScopeError.cpp`, `CScpiCommandBuilder.cpp`
- `plugins/PluginKeysightDSOX2012A/` (`.pro`, `.h`, `.cpp`), then one dir per
  fleet model (Tek MDO34, R&S RTM3004, LeCroy WaveSurfer 42XS, …)
- `models/scope_models.json`, `models/<model>/scpi_map.json`
- `datasheets/<vendor>/<model>/…` + `datasheets/index.json`
- `tools/datasheet_fetch/` (Digi-Key + Mouser API clients)
- Top-level `ScopePlugins.pro` (subdirs) — mirrors `ELoad_R2` layout

## Reference Mapping (quick lookup)

`CIPowerSupplyPlugin`→`CIScopePlugin` · `CPowerSupplyManager`→`CScopeManager` ·
`PSErrorCode`→`ScopeErrorCode` · `Enum_OperatingMode`→`Enum_AcquisitionMode` ·
`S_PowerSupplyCapabilities`→`S_ScopeCapabilities` · `setOVP/setUVP`→
`setVerticalScale/setTimebaseScale/setTriggerLevel/…` · `VisaHelper` reused ·
`.pro`/`Q_PLUGIN_METADATA`/`sendCommand`/`buildCommand`/`parse…Error` pattern
retained per plugin.

## Verification

- Build: `qmake && make` at repo root produces `ScopeCore` in `../lib` and each
  plugin `.so`/`.dll` in `../plugins`.
- `CScopeManager::loadPlugins("../plugins")` lists every built plugin and emits
  `pluginLoaded` (unit test + demo run).
- Simulator round-trip: `createInstance` → `connect` (`*IDN?`) →
  configure vertical/timebase/trigger → `single` → `captureWaveform` returns a
  correctly scaled `S_Waveform`; error paths return the expected
  `ScopeErrorCode`.
- Hardware smoke test on one LXI scope from the inventory reproduces the
  simulator flow.
- `ctest`/Qt Test suite green; generated-command test matches each
  `scpi_map.json`.
