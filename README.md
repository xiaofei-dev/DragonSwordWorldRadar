# DragonSwordWorldRadar

DragonSwordWorldRadar is a modular radar for **DragonSword Awakening**. This repository snapshot contains the complete development source for version `0.4.0-dev9-performance1.8-singlebridge1` in the original repository layout.

## Repository layout

```text
src/ue4ss/          UE4SS Lua producer
src/overlay/        C# WinForms Overlay
src/installer/      installer entry point and C# installer core
src/host/           hidden watcher and game-bound host
src/tools/          diagnostics collector
resources/          release-time defaults
build/              source verification, compile tests, and release packaging
docs/               architecture and performance notes
vendor/              required local build/runtime dependencies
```

The installable package uses a different layout (`scripts/`, `host/`, `installer/`, and `src/overlay/`). `build/Build-Release.ps1` performs that source-to-release mapping. Do not develop directly inside a deployed Mod folder.

## Build and validation

Run from Windows PowerShell 5.1:

```powershell
& .\build\Verify-Source.ps1
& .\build\Compile-Source.ps1
& .\build\Test-Refactor.ps1
& .\build\Build-Release.ps1
```

The build creates `dist/DragonSwordWorldRadar-v0.4.0-dev9-performance1.8-singlebridge1.zip`. Windows PowerShell 5.1 `Add-Type` is the authoritative compiler path because installation uses the same compiler.

---

## 1.8 single-bridge design

The following 1.7 performance baseline is unchanged:

- 24 ms minimap position sampling.
- 24 ms active world-map transform sampling; it stops when the world map closes.
- 250 ms low-frequency UObject/control sampling.
- 24 ms active Overlay timer, with the existing lower-frequency idle/background modes.
- 20 XY and 10 Z movement thresholds.
- Exactly the existing three textual `LoopAsync` registrations; no new worker loop, prime-number staggering, or ThreadPool polling chain.
- Cached Pawn fast path, C# treasure selection, save-filtered local catalog, normal buffered save-snapshot flush, delayed full-EXE key scan, and one-time hidden world-map prepaint.

The old runtime path was:

```text
UE4SS Lua
├─ radar_motion_a/b.dat     (high-frequency movement)
└─ radar_state_a/b.json     (low-frequency duplicate state)

Overlay
├─ fixed-record parser
└─ JSON parser + static/motion fallback merge
```

The 1.8 path is:

```text
UE4SS Lua
└─ radar_motion_a/b.dat     (all live control and motion data)

Overlay
├─ fixed protocol-v2 parser
├─ local treasure catalog
├─ local nine-boss catalog
└─ save/Boss availability filtering
```

### Motion protocol v2

Each alternating Motion slot is one fixed 27-field ASCII record. It contains:

- leading and trailing sequence numbers;
- explicit protocol version and Lua generation;
- enabled state and `radar` / `world` / `disabled` mode;
- height, treasure-type, treasure-layer, and Boss-layer display switches;
- text scale;
- player XYZ, Z validity, and radar radius;
- all world-map transform fields required while the map is open.

The explicit protocol version prevents an old 21-field record or partially upgraded installation from being interpreted with shifted fields. The existing two-slot sequence validation still rejects partial rewrites.

### Work removed from the runtime

1.8 removes:

- the Lua Static JSON builder and one-second heartbeat write;
- Static Bridge Boss serialization;
- `radar_state_a.json`, `radar_state_b.json`, and the legacy single state file as active IPC;
- the Overlay's 200 ms Static file poll;
- `JavaScriptSerializer` and the `System.Web.Extensions` dependency;
- Static/Motion generation matching, fallback-state merging, and duplicate redraw comparison;
- the Lua `boss_tracker.lua` runtime module.

The 250 ms Lua callback now reads only the low-frequency UObject/control state needed to refresh Pawn context, detect map mode, and update minimap radius. It does not build JSON or write a second bridge.

### Local static data ownership

- The Overlay loads `data/generated/treasures.lua` directly and applies save filtering locally.
- The Overlay loads `data/generated/bosses.lua` directly, validates exactly nine unique Boss IDs, and applies `tb_actor_respawn` availability locally.
- The compatibility wrappers `scripts/treasures.lua` and `scripts/bosses.lua` remain in the source package, but normal 1.8 runtime code does not require either catalog in Lua.

### Failure behavior

- A Motion frame older than 2500 ms is treated as stale; the Overlay hides instead of continuing to draw obsolete coordinates.
- New valid Motion data restores the Overlay automatically.
- Legacy Static files are deleted only as upgrade/session cleanup; they are never read or written as active state.
- Catalog, save, renderer, timer, process, and geometry failures remain isolated and rate-limited.

## World-map behavior

The accepted immediate-map behavior remains:

```text
hide Overlay
→ resize to the game client with SWP_NOCOPYBITS
→ one hidden Invalidate() + Update()
→ reveal immediately
```

There is no fixed one-second reveal delay and no recurring warm-up render loop. The first failed active-map sample stops full-map production and restores the small radar mode.

## Implemented layers

### Treasures

- Minimap and world-map markers generated locally from the game PAK.
- SQLCipher save filtering, overrides, aliases, type colors, nearest marker, and height indicator.
- Player-height comparison offset: `-150`.
- Until the first complete save snapshot is available, treasure visibility is unknown and treasure points remain hidden; startup does not flash every point.

### World bosses

- Exactly nine fixed locations generated locally from game data.
- Availability comes from `tb_actor_respawn`; no streamed `Character` scan or HP hook is used.
- Respawn cycle 106 uses the verified daily 09:00 local reset.
- Boss coordinates are loaded by the Overlay; Lua no longer serializes the same fixed records every second.

## Logging and diagnostics

Configure logging in `scripts/config.lua`:

```lua
use_logging = true,
debug_logging = false,
diagnostic_perf_interval_seconds = 5,
```

Normal use should keep `debug_logging = false`.

| Purpose | Path |
|---|---|
| Lua normal-use log | `runtime/logs/DragonSwordWorldRadar.Lua.Use.log` |
| Lua debug/performance log | `runtime/logs/DragonSwordWorldRadar.Lua.Debug.log` |
| Overlay normal-use/error log | `runtime/logs/DragonSwordWorldRadar.Overlay.Use.log` |
| Overlay debug/performance log | `runtime/logs/DragonSwordWorldRadar.Overlay.Debug.log` |

Overlay debug output reports the single Bridge's read rate, complete frames, redraw-producing frames, suppressed frames, timer gaps, paint gaps, CPU, and working set. `overlayPaintFps` is the WinForms paint rate, not the game's Present FPS.

`Collect-Diagnostics.cmd` captures both Motion slots, logs, generated data, metadata, process state, UE4SS log tail, and relevant source hashes. It no longer collects active Static Bridge files.

## Installation

1. Close the game and all existing radar processes.
2. Extract the complete folder to `Mods/DragonSwordWorldRadar`.
3. Run `Install.cmd` outside a ZIP preview.
4. Require these markers:

```text
OVERLAY_COMPILE_OK
INSTALLER_COMPILE_OK
SESSION_WATCHER_READY
INSTALL_COMPLETE
```

5. Start the game normally. Press **F7** to enable and **F8** to disable.

The installer preserves a valid `scripts/config.lua`, `data/treasure_overrides.txt`, and unrelated `mods.txt` entries. It also removes the three exact 1.7 source files retired by the single-Bridge architecture, so a normal full-folder overwrite followed by `Install.cmd` does not retain duplicate C# models or the old Lua Boss tracker.

## Validation status

The release preparation validates Lua syntax, actual stubbed execution of disabled/radar/world protocol-v2 records, protocol positive and negative cases, C# lexical structure and Windows PowerShell 5.1 CodeDOM compatibility patterns, JSON/XML structure, PowerShell delimiter structure, Boss catalog shape, source invariants, manifest hashes, ZIP CRC, path safety, duplicate entries, extracted-byte equality, and bundled binary hashes.

Windows PowerShell 5.1 `Add-Type` compilation and in-game FPS/frame-time testing are not available in the preparation environment. `Install.cmd` remains the authoritative Windows compile gate, and same-route game testing remains the authoritative performance gate. The expected gain from removing Static Bridge is lower periodic IPC/JSON work and simpler failure behavior; no specific FPS increase is claimed.

## Game updates

Installation records the game executable and generated-data PAK fingerprint in `metadata/install-state.json`. If the game changes, the watcher requests reinstallation rather than using stale generated data.

## License

GPL-3.0. See `THIRD_PARTY_NOTICES.txt` and `licenses/` for third-party components.
